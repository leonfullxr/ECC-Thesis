# Compile

```bash
export CXXFLAGS="-I/home/leon/sw/include"
export LDFLAGS="-L/home/leon/sw/lib -Wl,-rpath=/home/leon/sw/lib"
PREFIX=/home/leon/sw GMP_PREFIX=/home/leon/sw
export LD_LIBRARY_PATH="/home/leon/sw/lib:$LD_LIBRARY_PATH"
g++ -std=c++17 \                                                         ─╯
    -I/home/leon/sw/include \
    src/rsa.cpp \      
    -L/home/leon/sw/lib \
    -Wl,-rpath=/home/leon/sw/lib \
    -lntl -lgmp -pthread \
    -o ./bin/RSA
```

## 1. Pushing your RSA branch up to the remote

Assuming you’ve done work on your local `rsa-implementation` branch:

```bash
# 1. Make sure you’re on your RSA branch:
git checkout rsa-implementation

# 2. Stage and commit your work:
git add .
git commit -m "Add RSA implementation"

# 3. Push it to origin (remote):
git push origin rsa-implementation
```

Now you can open a Pull Request (PR) on your Git host (GitHub/GitLab/Bitbucket/etc.) **from** `rsa-implementation` **into** `main`. That PR is how you’ll ultimately get those commits merged into `main`.

---

## 2. Merging your RSA branch into `main`

Once your RSA PR is approved, you have two equivalent options:

### A. Via the Git host UI (*recommended*)

1. Click **Merge** (or **Squash and merge**) in the PR on GitHub/GitLab.
2. That action will merge the commits on `rsa-implementation` into `main` and update the remote `main` branch.

### B. Locally, then push

```bash
# 1. Switch to main
git checkout main

# 2. Make sure it’s up to date
git pull origin main

# 3. Merge in your feature branch
git merge rsa-implementation

# 4. Push the updated main
git push origin main
```

---

## 3. Bringing your documentation changes into your RSA branch

Suppose you have a separate branch `docs-updates` with commits that you also want in your RSA branch. You have two principal choices:

### A. **Merge** `docs-updates` into `rsa-implementation`

```bash
# 1. Fetch the latest:
git fetch origin

# 2. Check out your RSA branch:
git checkout rsa-implementation

# 3. Merge in the docs branch:
git merge origin/docs-updates
```

Resolve any conflicts, then:

```bash
# 4. Push the result
git push origin rsa-implementation
```

This creates a merge commit on `rsa-implementation` that brings in all the documentation commits.

### B. **Rebase** your RSA branch onto `docs-updates`

```bash
git fetch origin
git checkout rsa-implementation
git rebase origin/docs-updates
```

This “rewrites” your RSA commits so they sit on top of the documentation commits. After a rebase you’ll need:

```bash
git push --force-with-lease origin rsa-implementation
```

> **When to merge vs. rebase?**
>
> * **Merge** if you want a record of when you combined the two branches (adds a merge commit).
> * **Rebase** if you prefer a linear history (no merge commit), but only on branches that aren’t yet shared—or if you’re comfortable force-pushing.

---

## 4. Propagating docs → main → RSA (alternative workflow)

A common pattern is:

1. **Merge** `docs-updates` into `main` first.
2. **Merge** (or **rebase**) **latest** `main` into your `rsa-implementation` branch, so that RSA sees the docs:

   ```bash
   git checkout main
   git pull origin main          # get the merged docs
   git checkout rsa-implementation
   git merge main               # or `git rebase main`
   git push origin rsa-implementation
   ```

That way you only ever merge docs into one place (`main`), and then pull them downstream into every feature branch that needs them.

---

### TL;DR

* **Feature branches**: one per logical change (e.g. `docs-updates`, `rsa-implementation`).
* **To get a feature into `main`**: open (or locally run) a **merge** or **rebase** from your feature-branch into `main`, then `git push origin main`.
* **To share commits between two feature branches**: check out the branch that needs the commits and **merge** (or **rebase**) the other branch into it.

