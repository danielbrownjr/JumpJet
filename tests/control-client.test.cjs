// SPDX-License-Identifier: GPL-3.0-or-later
"use strict";

const test = require("node:test");
const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const {
  Client,
  ControlError,
  controllerLabel,
  constraintMessage,
} = require("../components/jj_portal/web/control-client.js");

function state(generation, revision, leaseId) {
  return {
    boot_id: "boot-a",
    state_revision: revision,
    mode: "off",
    controller: "none",
    control: {
      generation,
      lease_id: leaseId,
      authority: "none",
      inhibit: "none",
    },
  };
}

function response(payload, ok = true) {
  return {ok, json: async () => payload};
}

function scriptedFetch(script, calls) {
  return async (path, options) => {
    const body = JSON.parse(options.body);
    calls.push({path, body});
    const next = script.shift();
    assert.ok(next, "unexpected fetch");
    return typeof next === "function" ? next(path, body) : next;
  };
}

test("resume acquires authority then performs authoritative refresh", async () => {
  const calls = [];
  const ready = [];
  const fetch = scriptedFetch([
    response(state(2, 2, "a".repeat(32))),
    response(state(2, 3, "a".repeat(32))),
  ], calls);
  const client = new Client({fetch, owner: "phone", onReady: value => ready.push(value)});
  await client.resume(false);
  assert.deepEqual(calls.map(call => call.path), [
    "/api/v2/control/acquire",
    "/api/v2/control/refresh",
  ]);
  assert.equal(calls[0].body.takeover, false);
  assert.equal(calls[1].body.control_generation, 2);
  assert.equal(client.ready, true);
  assert.deepEqual(ready, [false, true]);
});

test("takeover is sent only after explicit caller confirmation", async () => {
  const calls = [];
  const fetch = scriptedFetch([
    response(state(2, 2, "b".repeat(32))),
    response(state(2, 3, "b".repeat(32))),
  ], calls);
  const client = new Client({fetch, owner: "phone"});
  await client.resume(true);
  assert.equal(calls[0].body.takeover, true);
});

test("mutation carries causal tokens and requested final semantic state", async () => {
  const calls = [];
  const fetch = scriptedFetch([
    response(state(2, 2, "a".repeat(32))),
    response(state(2, 3, "a".repeat(32))),
    response({...state(2, 4, "a".repeat(32)), mode: "manual"}),
  ], calls);
  const client = new Client({fetch, owner: "phone", uuid: () =>
    "12345678-1234-1234-1234-123456789abc"});
  await client.resume(false);
  await client.mutate("manual", 46);
  assert.deepEqual(calls[2].body, {
    lease_id: "a".repeat(32),
    control_generation: 2,
    expected_state_revision: 3,
    request_id: "12345678-1234-1234-1234-123456789abc",
    action: "manual",
    target_c: 46,
  });
});

test("control loss disables mutation until reacquisition and refresh", async () => {
  const calls = [];
  const fetch = scriptedFetch([
    response(state(2, 2, "a".repeat(32))),
    response(state(2, 3, "a".repeat(32))),
    response({error: "control_authority_lost", state: state(3, 4)}, false),
  ], calls);
  const client = new Client({fetch, owner: "phone"});
  await client.resume(false);
  await assert.rejects(client.mutate("off"), error =>
    error instanceof ControlError && error.code === "control_authority_lost");
  assert.equal(client.ready, false);
  await assert.rejects(client.mutate("off"), error =>
    error.code === "control_authority_required");
  assert.equal(calls.length, 3);
});

test("late mutation response from an older generation cannot replace current state", async () => {
  let resolveMutation;
  const mutationResponse = new Promise(resolve => { resolveMutation = resolve; });
  const calls = [];
  const fetch = scriptedFetch([
    response(state(2, 2, "a".repeat(32))),
    response(state(2, 3, "a".repeat(32))),
    () => mutationResponse,
    response(state(3, 5, "b".repeat(32))),
    response(state(3, 6, "b".repeat(32))),
  ], calls);
  const client = new Client({fetch, owner: "phone"});
  await client.resume(false);
  const late = client.mutate("off");
  await client.resume(false);
  resolveMutation(response({...state(2, 4, "a".repeat(32)), mode: "manual"}));
  await late;
  assert.equal(client.generation, 3);
  assert.equal(client.revision, 6);
  assert.equal(client.state.mode, "off");
});

test("page foreground and restored-page events reacquire and refresh", () => {
  const page = fs.readFileSync(path.join(
    __dirname, "../components/jj_portal/web/control.html"), "utf8");
  assert.match(page, /visibilitychange/);
  assert.match(page, /visibilityState === "visible"\) resume\(false\)/);
  assert.match(page, /pageshow/);
  assert.match(page, /event\.persisted\) resume\(false\)/);
});

test("primary controller label follows effective authority, not configured mode", () => {
  const retainedManual = state(3, 5);
  retainedManual.mode = "manual";
  retainedManual.controller = "remote";
  retainedManual.control.authority = "none";
  assert.equal(controllerLabel(retainedManual), "Manual · None");
  retainedManual.control.authority = "reacquiring";
  assert.equal(controllerLabel(retainedManual), "Manual · Reacquiring");
  retainedManual.mode = "automatic";
  retainedManual.control.authority = "automatic";
  assert.equal(controllerLabel(retainedManual), "Automatic · PrusaLink");
});

test("primary constraints are operator-readable while unknown codes stay generic", () => {
  assert.equal(constraintMessage("control_no_authority"),
               "Control inhibited: remote controller disconnected");
  assert.equal(constraintMessage("printer_unavailable"),
               "Heating inhibited: PrusaLink status unavailable or stale");
  assert.equal(constraintMessage("not_commissioned"),
               "Heating unavailable: product not commissioned");
  assert.equal(constraintMessage("future_internal_code"),
               "Heating inhibited: see advanced diagnostics");
});
