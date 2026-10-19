# Publish VitalWatch to GitHub (Windows CMD)

**Repository owner:** `yusufthecoder474` · **proposed repository name:** `VitalWatch`.

The uploaded ZIP did not contain a `.git` directory. The user has already initialized a local Git repository on the `main` branch and staged the existing source files. The commands below are designed to preserve that work.

## 1. Copy the new documentation before committing

Extract the documentation ZIP into your **existing extracted project root** (the directory containing `CMakeLists.txt`), accepting replacement of `README.md` when prompted. Do **not** extract a second nested project folder into the repository.

Expected files:

- `README.md` (replaces the older README)
- `docs/DEMO_GUIDE.md` (new)
- `docs/GITHUB_PUBLISHING.md` (new)

Existing code and existing docs should remain unchanged.

## 2. Stage the updated files

Open CMD inside the extracted project root:

```cmd
cd /d D:\Downloads\VitalWatch\VitalWatch
git add README.md docs\DEMO_GUIDE.md docs\GITHUB_PUBLISHING.md
git status
```

If you have **not committed yet**:

```cmd
git commit -m "Initial commit: VitalWatch C++17 simulator and documentation"
```

If you **already committed** the original README, make a normal second commit after updating docs:

```cmd
git commit -m "docs: improve README and add demonstration guide"
```

Avoid `git init` again: the existing local repository is already initialized.

## 3. Create an empty public repository

Open https://github.com/new and create repository `VitalWatch` under `yusufthecoder474`.

- Visibility: Public, if intended for recruiters.
- **Do not** pre-create a README or `.gitignore` on GitHub.
- License: choose none until the owner decides the license terms.

Check that no build outputs, credentials, private data or unexpected files are staged before publishing.

## 4. Connect and push

```cmd
git remote -v
```

If there is no `origin` yet:

```cmd
git remote add origin https://github.com/yusufthecoder474/VitalWatch.git
```

Then:

```cmd
git push -u origin main
```

Sign in through Git Credential Manager if prompted. **Never paste GitHub passwords or access tokens into a chat.**

If the `origin` already exists but points somewhere else, inspect it before changing it. If push is rejected because the remote has its own initial commit, stop and inspect the divergence rather than force pushing.

## 5. Verify publication

Open the URL **after the repository exists**:

https://github.com/yusufthecoder474/VitalWatch

Confirm the README renders, documentation links work, and `src/`, `include/`, `tests/`, `data/`, and `evidence/` are visible. Do not claim a GitHub Actions CI pass because no workflow exists in this version.

## 6. Add to the resume

Only after verifying the public GitHub page:

```latex
\href{https://github.com/yusufthecoder474/VitalWatch}{GitHub}
```

## License decision

There is no LICENSE file in this package. Ask the owner to select MIT, Apache-2.0, ISC, or proprietary terms before adding a license. **Do not silently add an open-source license.**
