# Branch Retention Policy

This repository uses branches for active work and tags for frozen historical checkpoints.

The goal is to keep the branch list readable without losing useful history.

## Namespace rules

Use separate namespaces for moving branches and frozen tags:

- `feat/...`, `fix/...`, `docs/...`, `test/...`: active branches
- `parked/...`: paused or superseded branches that may move again
- `backup/...`: temporary recovery branches
- `archive/...`: archival tags only

Do not create branches under `archive/...`. Keeping archival tags and retained branches in separate namespaces avoids ambiguous ref resolution.

## Active branches

Keep normal feature, fix, documentation, and test branch names while work or review is active.

Examples:

- `feat/...`
- `fix/...`
- `docs/...`
- `test/...`

Do not rename an active branch merely for housekeeping.

## Merged branches

JumpJet may use squash or rebase merging for completed pull requests. Under either strategy, the original PR head commit is not expected to become reachable from `main`. The archival tag is therefore the preservation mechanism and is mandatory before the merged branch is discarded.

Capture the PR head SHA before merging, especially if GitHub is configured to delete merged head branches automatically:

```bash
git fetch --tags --prune origin
PR_HEAD=$(git rev-parse origin/<branch>)
printf '%s\n' "$PR_HEAD"
```

After the pull request is confirmed merged:

1. Create an annotated archival tag at the captured PR head SHA.
2. Push the archival tag.
3. Verify the remote tag exists and resolves to the intended commit.
4. Delete the merged remote branch if it still exists.

Suggested tag format:

```text
archive/pr-<number>-<short-description>
```

Example:

```bash
git fetch --tags --prune origin
PR_HEAD=$(git rev-parse origin/feat/phase1-product-skeleton)

# Merge through the normal GitHub workflow, then preserve the original PR head.
git tag -a archive/pr-<number>-phase1-product-skeleton "$PR_HEAD" \
  -m "Archive PR #<number>: Phase 1 product skeleton"

git push origin archive/pr-<number>-phase1-product-skeleton
git fetch --tags --prune origin
git rev-parse archive/pr-<number>-phase1-product-skeleton^{commit}

git push origin --delete feat/phase1-product-skeleton
```

If GitHub has already deleted the head branch automatically, use the PR metadata or merge event to recover the recorded head SHA and create the archival tag before doing any further cleanup.

Deleting the branch ref does not delete commits referenced by the archival tag.

## Abandoned or paused branches

If a branch contains unique work that may be useful later but should not remain in the active branch namespace, rename it under:

```text
parked/...
```

Examples:

```text
parked/phase1-pre-realign
parked/mechanical-layout-experiment
```

Do not park branches merely because they are old. Park them when the work is intentionally paused or superseded and may reasonably resume.

## Backup branches

Use `backup/...` only for temporary safety checkpoints created before risky Git operations, rebases, branch realignment, history repair, or similar work.

Example:

```text
backup/phase1-local-before-realign-YYYYMMDD
```

A backup branch should remain until the repair or migration is fully validated.

After that milestone closes, either:

- create an archival tag and delete the backup branch, or
- rename it to `parked/...` if retaining a movable branch reference remains useful.

## Safety rule

Never delete or rename a branch containing unique unmerged work until the required history is positively confirmed to exist somewhere else.

Before any verification, refresh both remote branches and tags:

```bash
git fetch --tags --prune origin
```

Then verify the preservation mechanism appropriate to the integration strategy.

For a merge-commit workflow, target-branch reachability may be sufficient:

```bash
git branch -r --contains <commit>
```

For squash or rebase merges, do not expect the original PR head to be reachable from `main`. Verify the archival tag instead:

```bash
git rev-parse archive/pr-<number>-<short-description>^{commit}
git tag --contains <commit>
```

For paused or abandoned work, verify that a retained branch or archival tag still points to the required commit before deleting the original branch.

`git log origin/main..origin/<branch>` is useful for understanding branch-only commits, but under squash or rebase merging it will continue to show the original PR commits even after their changes are integrated. Do not use that output alone to decide whether a merged branch may be deleted.

## Protect archival tags

Git tags are mutable unless repository policy prevents mutation. Because `archive/...` tags are the long-term preservation record, configure a GitHub ruleset for:

```text
refs/tags/archive/*
```

The ruleset should prevent deletion and force updates except through an explicitly authorized recovery process.

## Hardware and safety-critical history

JumpJet hardware and firmware development is safety-related, so meaningful design checkpoints are retained more conservatively than ordinary software branches.

Temporary recovery branches may remain through the next major milestone before archival cleanup. Do not remove historical references that may be needed to reconstruct:

- schematic or PCB design decisions,
- heater and fan safety architecture,
- component or hardware characterization,
- manufacturing baselines,
- released firmware states,
- safety validation evidence.

If there is doubt about whether a checkpoint may matter to future fault analysis or hardware reconstruction, retain it until the relevant milestone or release is closed.

## General rule

Branches represent work that may still move.

Tags represent history that should not.
