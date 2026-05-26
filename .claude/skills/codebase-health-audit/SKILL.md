---
name: codebase-health-audit
description: Comprehensive codebase health audit covering bugs, dead code, coupling, performance, security, and accessibility. Run on any project at version checkpoints. Triggers on: "audit", "health check", "code quality", "bug hunt", "屎山", "验收", "排查", "code review all", "quality gate", "cleanup pass".
metadata:
  type: skill
  version: 1.0.0
  scope: universal
---

# Codebase Health Audit

Universal multi-dimensional codebase health audit. Designed to be project-agnostic — adapts to any language, framework, or stack.

## When to Use

- At version checkpoints (before tagging a release)
- After a major feature sprint
- When inheriting or onboarding to a codebase
- Before planning the next development phase
- When the user says "audit my codebase", "health check", "find bugs and dead code", "质量检查", "屎山排查"

## Audit Dimensions (6-Pillar Framework)

Each pillar is scored: **PASS / WARN / FAIL**

### Pillar 1: Build & Type Safety
- [ ] Type checker passes with zero errors (`tsc --noEmit`, `mypy`, `cargo check`, etc.)
- [ ] Production build succeeds
- [ ] Test suite passes (if exists)
- [ ] No suppressed/ignored type errors without documented justification

### Pillar 2: Code Smell & Dead Code
- [ ] No duplicate function/component definitions across files (>5 similar lines)
- [ ] No commented-out code blocks (use git history, not comments)
- [ ] No unused exports (grep `export` and verify each has at least one import)
- [ ] No unused imports
- [ ] No module-level mutable variables (React: no `let` outside components)
- [ ] No magic strings/numbers — named constants preferred

### Pillar 3: Coupling & Architecture
- [ ] State management modules don't leak into unrelated components
- [ ] No circular imports/dependencies
- [ ] Component files are under 400 lines (split if larger)
- [ ] Store/state modules have single responsibility
- [ ] Shared utilities are in dedicated `lib/` or `utils/` files, not duplicated

### Pillar 4: Correctness & BUG
- [ ] No unsafe type assertions (`as` in TS, `unsafe` in Rust) without guards
- [ ] No null/undefined access without guard clauses
- [ ] All `useEffect` have proper cleanup (return function)
- [ ] All list renders have unique `key` props
- [ ] Async operations have timeout/error handling (no infinite loading)
- [ ] Server: input validation on all external data paths
- [ ] Server: authentication enforced on all sensitive operations
- [ ] No race conditions in shared mutable state

### Pillar 5: Performance
- [ ] List items use memoization (`React.memo`, `useMemo`) where beneficial
- [ ] No unnecessary re-renders (check with React DevTools Profiler)
- [ ] Large data structures use appropriate patterns (virtualization, pagination)
- [ ] No memory leaks (unsubscribed listeners, uncleaned intervals)
- [ ] Server: connection pooling, buffer size limits, heartbeat timeouts

### Pillar 6: Security & Accessibility
- [ ] No `dangerouslySetInnerHTML` / raw HTML injection
- [ ] No secrets/credentials in source code
- [ ] All user inputs validated and sanitized at system boundaries
- [ ] All icon-only buttons have `aria-label`
- [ ] All form inputs have associated `<label>`
- [ ] Keyboard navigation works (Tab order, Enter/Space on interactive elements)
- [ ] Focus visible indicators present (`:focus-visible`)

## Audit Execution Protocol

### Step 1: Scope Definition
Ask or determine:
- Which directories to audit (default: entire project)
- Which languages/frameworks are in scope
- Whether to run build verification or code-only review

### Step 2: Automated Checks (always run first)
```bash
# TypeScript
npx tsc --noEmit 2>&1

# Rust
cargo check 2>&1

# C++ (CMake)
cmake --build build/ --config Debug 2>&1
ctest -C Debug --test-dir build/ 2>&1

# Bundle analysis
npm run build 2>&1
```

### Step 3: Grep Sweep (common patterns across all languages)

Dead exports (TS):
```bash
# List all exports, then verify each is imported somewhere
grep -rn "^export " src/ --include="*.ts" --include="*.tsx"
```

Unused imports (TS):
```bash
npx eslint --rule 'no-unused-vars: error' 2>&1 || true
```

Commented-out code:
```bash
grep -rn "//.*\(function\|const\|let\|var\|import\|export\|class\|if\|for\|while\)" src/ --include="*.ts" --include="*.tsx"
```

Module-level mutable state (TS/JS):
```bash
grep -rn "^let " src/ --include="*.ts" --include="*.tsx" | grep -v "// \|test\|\.test\."
```

Unsafe patterns:
```bash
grep -rn "as " src/ --include="*.tsx" | grep -v "as const\|as string\|as number"
grep -rn "unsafe " src/ --include="*.rs"
grep -rn "unwrap()" src/ --include="*.rs"
```

Security:
```bash
grep -rn "dangerouslySetInnerHTML\|innerHTML\|eval(" src/ --include="*.tsx" --include="*.ts"
grep -rn "password\|secret\|token\|key\|api_key" src/ --include="*" | grep -v "\.env\|node_modules\|\.git"
```

Accessibility:
```bash
grep -rn "<button" src/ --include="*.tsx" | grep -v "aria-label"
grep -rn "<input" src/ --include="*.tsx" | grep -v "<label\|aria-label\|aria-labelledby"
```

### Step 4: Manual Code Review
Read each source file. Check for:
- Logic errors in conditional branches
- Missing error handling in async operations
- State management correctness (setters match getters)
- Component lifecycle issues (cleanup, stale closures)

### Step 5: Severity Grading

| Grade | Criteria | Action |
|-------|----------|--------|
| **P0 (Critical)** | Security vulnerability, data loss, crash, infinite hang | Fix immediately |
| **P1 (High)** | Functional bug, memory leak, incorrect behavior | Fix in current sprint |
| **P2 (Medium)** | Code smell, duplication, poor pattern | Schedule for next version |
| **P3 (Low)** | Style inconsistency, minor optimization | Backlog |

### Step 6: Report Output

Generate `docs/audit-report-YYYY-MM-DD.md` with:
1. Executive summary (PASS/WARN/FAIL per pillar)
2. P0/P1 issues with file:line references
3. P2 recommendations
4. Build verification results
5. Bundle size baseline (if frontend)

## Post-Audit Actions

1. Fix all P0 and P1 issues
2. Create tasks for P2 items in next version plan
3. Update CLAUDE.md/CODEX.md with new quality baseline
4. Commit audit report to docs/

## Customization Hooks

For project-specific checks, add to the checklist:
- Framework-specific patterns (e.g., React: check for missing `key` props)
- Language-specific gotchas (e.g., C++: check for raw `new`/`delete`)
- Team conventions (e.g., "no console.log in production code")
