# Contributing to H5CPP

Fixes and contributions are welcome, including external feature branches and pull requests. All contributions are accepted under the MIT license.

## Issue naming

Issues should follow the form:

```text
<category>, <description>
```

Examples:

```text
ci, add macOS AppleClang runner
fix, resolve HDF5 discovery on Homebrew
feature, add compiler metadata contract
refactor, simplify datatype synthesis
docs, update migration notes
```

Categories:

```text
fix
feature
refactor
ci
docs
test
legal
cleanup
```

## Branch naming

Feature branches should be created from the current `staging` branch and follow the form:

```text
<#issue>-<category>-<description>
```

Examples:

```text
138-ci-add-macos-appleclang-runner
139-fix-hdf5-homebrew-discovery
140-refactor-datatype-synthesis
```

New work should always start from the current `staging` branch:

```bash
git fetch origin
git checkout staging
git pull --ff-only origin staging
gh issue develop <#issue> --checkout
```

If the generated branch name differs from the convention, rename or recreate it before pushing.

## Commit format

Each commit should follow this pattern:

```text
[#issue]:author:category, description
```

Examples:

```text
[#138]:svarga:ci, add macOS AppleClang runner
[#139]:svarga:fix, resolve HDF5 discovery on Homebrew
[#140]:svarga:refactor, simplify datatype synthesis
```

Keep commits focused. Prefer several precise commits over one heroic commit that tries to solve the universe and accidentally invents another one.

## Rebase-based development

H5CPP uses a rebase-based feature-branch workflow. Feature branches are carried forward by rebasing onto the current `staging` branch rather than merging `staging` into the feature branch.

Before opening or updating a pull request:

```bash
git fetch origin
git checkout <feature-branch>
git rebase origin/staging
```

Resolve conflicts locally, rerun the relevant build and test matrix where possible, then force-push safely:

```bash
git push --force-with-lease
```

Use `--force-with-lease`, not plain `--force`; we are civilized barbarians.

## Pull requests

Once the feature branch is complete, open a pull request targeting `staging`.

A pull request should:

- reference the issue it resolves
- follow the branch and commit naming convention
- keep the change scoped to the issue
- pass the relevant CI jobs
- document user-visible behavior changes

The preferred merge target for active development is `staging`. Release branches are promoted from `staging` after validation.

## Documentation — alias-first docstrings

H5CPP's API surface is built on overload sets (`h5::write` has ~40 overloads, `h5::read` ~30, `h5::create` ~20). Each carries the same `file_path`, `dataset_path`, `offset`, `stride`, `count`, `block`, `dxpl`, template parameter, return type. Without discipline, the same `@param` block ends up duplicated across dozens of docstrings — and a wording change requires dozens of edits.

To keep docstrings DRY, the project uses a controlled vocabulary of Doxygen aliases defined in `docs/links/*.txt`. Each alias is a single-token expansion that produces a fully-formed `@param` / `@tparam` / `@return` / `@see` block.

The full vocabulary catalog, standard docstring template, and workflow for adding new aliases live at [`docs/aliases.md`](docs/aliases.md).

### The rule

When writing or refactoring a docstring:

1. If a `@param` / `@tparam` / `@return` line you're about to write would repeat **≥ 5 times** across the codebase, use an existing alias from `docs/links/h5cpp.txt` — or add a new one there and reference it via `\name`.
2. The chained-aliases line goes at the **end** of the comment, after any `\code` block. Order: `\par_*` → `\tpar_*` → `\returns_*` → `\sa_*`.
3. New aliases land in the same commit as their first use. Update `docs/aliases.md` so the catalog stays in sync.

The canonical example is `h5cpp/H5Dread.hpp`. The standard template is in [`docs/aliases.md`](docs/aliases.md).
