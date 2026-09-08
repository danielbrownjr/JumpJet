# Branch Retention Policy

This repository uses branches for active work and tags for frozen historical checkpoints.

The goal is to keep the branch list readable without losing useful history.

## Active branches

Keep normal feature, fix, documentation, and test branch names while work or review is active.

Examples:

- `feat/...`
- `fix/...`
- `docs/...`
- `test/...`

Do not rename an active branch merely for housekeeping.

## Merged branches

After a pull request is merged:

1. Confirm the PR is merged and the final head commit is known.
2. Create an annotated archival tag at the final PR head.
3. Push the tag.
4. Delete the merged remote branch.

Suggested tag format:

```text
archive/pr-<number>-<short-description>
```

Example:

```bash
git fetch origin

git tag -a archive/pr-<number>-<short-description> <final-pr-head-sha> \
  -m "Archive PR #<number>: <short description>"

git push origin archive/pr-<number>-<short-description>
git push origin --delete <merged-branch>
```

Deleting the branch ref does not delete commits referenced by the archival tag.

## Abandoned or paused branches

If a branch contains unique work that may be useful later but should not remain in the active branch namespace, rename it under:

```text
archive/...
```

Examples:

```text
archive/phase1-pre-realign
archive/mechanical-layout-experiment
```

Do not archive branches merely because they are old. Archive them when the work is intentionally paused or superseded.

## Backup branches

Use `backup/...` only for temporary safety checkpoints created before risky Git operations, rebases, branch realignment, history repair, or similar work.

Example:

```text
backup/phase1-local-before-realign-YYYYMMDD
```

A backup branch should remain until the repair or migration is fully validated.

After that milestone closes, either:

- create an archival tag and delete the backup branch, or
- rename it to `archive/...` if retaining a branch reference remains useful.

## Safety rule

Never delete or rename a branch containing unique unmerged commits until those commits are positively confirmed to exist somewhere else.

Before deleting a branch, verify at least one of the following:

- the commits are reachable from the merged target branch,
- an archival tag points to the required commit,
- another retained branch contains the commits.

Useful checks include:

```bash
git log origin/main..origin/<branch>
git branch -r --contains <commit>
git tag --contains <commit>
```

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
