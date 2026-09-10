# Jump Jet control authority

This is the control-authority and browser mutation contract. It does not grant
physical heater capability and does not replace the product safety contract.

## Orthogonal state

The API reports operating mode, active authority, control inhibit, safety fault,
and thermal state separately. A disconnected remote controller is represented
as `authority=none` and `inhibit=no_authority`; it is not a hardware fault.
Cooldown may therefore remain active while remote heat request and logical heat
permission are both off.

MANUAL demand is valid only under REMOTE authority. AUTOMATIC demand is owned by
product-local AUTOMATIC authority after an eligible atomic transition and is
not tied to the browser lease. OFF has no heating authority.

## Lease lifecycle

`POST /api/v2/control/acquire` accepts an operator-readable `owner` and an
explicit `takeover` boolean. It returns a server-generated opaque lease ID,
control generation, state revision, expiry, current eligibility/constraint,
and the complete authoritative state. A different active owner is rejected
with `control_authority_required` unless takeover is explicitly true.

Acquisition enters REACQUIRING and always clears prior MANUAL demand. The client
must then call `POST /api/v2/control/refresh` with the lease ID and generation.
Only that server-side handshake ends REACQUIRING; mutations sent before it are
rejected. This makes refresh an enforced protocol step rather than a browser
convention. The initial lease TTL is 60 seconds. Visible-page heartbeats renew
it for usability, but background JavaScript is not relied on for fail-cold
behavior.

On foreground/resume the browser discards its authority assumption and repeats
acquire plus refresh. Reconnection never restores a prior MANUAL request.

## Atomic mutations

`POST /api/v2/control/mutate` carries:

- `lease_id`
- `control_generation`
- `expected_state_revision`
- optional `request_id` UUID for bounded content-bound retry handling
- final semantic `action`: `off`, `manual`, `automatic`, or `clear_fault`
- `target_c` for `manual`

The server checks lease ownership, generation, revision, current product inputs,
interlocks, fault state, fan proof, PrusaLink eligibility where applicable, and
transition policy in one authority critical section. It commits the complete
mutation or nothing and returns authoritative resulting state. Request IDs are
cached in a bounded eight-entry window; an identical retry returns its original
result with current authoritative state, while reuse with different content
returns `control_request_conflict`. Historical state is never replayed as live
control demand.

Stable machine-readable failures are:

| Code | Meaning |
|---|---|
| `control_authority_required` | No usable authority, explicit takeover required, or refresh incomplete |
| `control_authority_lost` | The presented lease is absent, revoked, or expired |
| `control_generation_stale` | The request belongs to an older authority epoch |
| `state_revision_conflict` | Authoritative state changed before commit |
| `control_request_ineligible` | Current product conditions reject the complete request |
| `control_request_conflict` | A request UUID was reused with different content |
| `hardware_fault_latched` | A real safety fault requires its existing recovery semantics |
| `invalid_request` | The request schema is invalid |

Control conflicts use HTTP 409, hardware faults use 423, and malformed requests
use 400. Clients must use codes, not prose.

## Client ordering

State carries a boot ID, control generation, and revision. The control page
accepts a new boot ID only during explicit reacquisition, rejects regressive
generation/revision state, and ignores mutation responses whose request
generation is no longer current. A control-loss response disables all mutations
until another acquisition and refresh complete.

The primary controller label is derived from active authority, never merely from
configured mode. Operator-facing constraints are translated into readable
guidance; raw inhibit and constraint codes remain available in advanced
diagnostics for tooling and troubleshooting.

All control POST bodies must use `application/json`. General state GET responses
never disclose the opaque lease ID. The dedicated control page receives it only
from successful lease-bound responses.
