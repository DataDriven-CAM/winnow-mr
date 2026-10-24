# Winnow Monorepo

An overview of the **Winnow** ecosystem, featuring the high-performance FFI engine and its dashboard interface.

## 📂 Repository Structure

- `packages/winnow` — The core logic, C++ bindings, web sockets, and maybe FFI layer.
- `packages/winnow-ui` — The user interface and dashboard application.
- `cpp_modules/` — Shared native C++ module dependencies[uncommitted].

## 🛠️ Quick Start

This project uses **Bun Workspaces** to manage development lifecycles.

### 1. Install All Dependencies
Run this at the root to set up both JavaScript packages and compile the shared C++ dependencies:
```bash
cnpm install
```

### 2. Run Local Development
You can run individual workspaces from the root folder:

```bash
# Run the core FFI engine
cnpm --filter winnow dev

# Run the user interface
cnpm --filter winnow-ui dev
```

### 3. Testing
Execute tests across the entire workspace layout:
```bash
cnpm test packages/**/*
```

## 🚀 Releasing

We use a **unified release strategy**. Version numbers are synchronized across all `package.json` files. 

To trigger a production release and update the changelogs, push a version tag:
```bash
git tag v1.0.0
git push --tags
```
