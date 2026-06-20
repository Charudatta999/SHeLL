#!/bin/bash
# Integration tests for pipelines and redirections
# Run: bash tests/integration/pipelines/test_pipelines.sh [path/to/shellrepl]

SHELL_BIN="${1:-./build/bin/shellrepl}"
PASS=0
FAIL=0
TMPDIR=$(mktemp -d)

trap 'rm -rf "$TMPDIR"' EXIT

assert_stdout() {
    local desc="$1"
    local input="$2"
    local expected="$3"
    local actual
    actual=$(echo "$input" | timeout 5 "$SHELL_BIN" 2>/dev/null | sed 's/\[[^]]*\]\$//g' | sed '/^$/d')

    if [ "$actual" = "$expected" ]; then
        echo "  PASS: $desc"
        ((PASS++))
    else
        echo "  FAIL: $desc"
        echo "    expected: '$expected'"
        echo "    actual:   '$actual'"
        ((FAIL++))
    fi
}

assert_file_not_empty() {
    local desc="$1"
    local input="$2"
    local file="$3"

    rm -f "$file"
    echo "$input" | timeout 5 "$SHELL_BIN" >/dev/null 2>&1

    if [ -s "$file" ]; then
        echo "  PASS: $desc"
        ((PASS++))
    else
        echo "  FAIL: $desc (file missing or empty)"
        ((FAIL++))
    fi
    rm -f "$file"
}

assert_file() {
    local desc="$1"
    local input="$2"
    local file="$3"
    local expected="$4"

    rm -f "$file"
    echo "$input" | timeout 5 "$SHELL_BIN" >/dev/null 2>&1

    if [ ! -f "$file" ]; then
        echo "  FAIL: $desc (file not created)"
        ((FAIL++))
        return
    fi

    local actual
    actual=$(cat "$file")
    if [ "$actual" = "$expected" ]; then
        echo "  PASS: $desc"
        ((PASS++))
    else
        echo "  FAIL: $desc"
        echo "    expected: '$expected'"
        echo "    actual:   '$actual'"
        ((FAIL++))
    fi
    rm -f "$file"
}

echo "=== Pipeline Tests ==="

assert_stdout "2-stage pipeline" \
    "/bin/echo hello | cat" \
    "hello"

assert_stdout "3-stage pipeline" \
    "/bin/echo abc | cat | cat" \
    "abc"

assert_stdout "pipeline with grep" \
    "/bin/echo match_me | grep match_me" \
    "match_me"

assert_stdout "pipeline grep filters" \
    "printf 'aaa\nbbb\nccc\n' | grep bbb" \
    "bbb"

assert_stdout "pipeline wc -l" \
    "printf 'a\nb\nc\n' | wc -l" \
    "3"

echo ""
echo "=== Redirect Tests ==="

assert_file "stdout > file" \
    "/bin/echo redir_out > $TMPDIR/out.txt" \
    "$TMPDIR/out.txt" \
    "redir_out"

assert_file "stdout >> append" \
    "/bin/echo line1 > $TMPDIR/app.txt
/bin/echo line2 >> $TMPDIR/app.txt" \
    "$TMPDIR/app.txt" \
    "line1
line2"

assert_file "clobber >|" \
    "/bin/echo clobbered >| $TMPDIR/clob.txt" \
    "$TMPDIR/clob.txt" \
    "clobbered"

assert_file "both &> file" \
    "/bin/echo both_out &> $TMPDIR/both.txt" \
    "$TMPDIR/both.txt" \
    "both_out"

assert_file "both append &>> file" \
    "/bin/echo first &> $TMPDIR/bapp.txt
/bin/echo second &>> $TMPDIR/bapp.txt" \
    "$TMPDIR/bapp.txt" \
    "first
second"

# stdin <
/bin/echo "from_file" > "$TMPDIR/in.txt"
assert_stdout "stdin < file" \
    "cat < $TMPDIR/in.txt" \
    "from_file"

# stderr 2>
assert_file_not_empty "stderr 2> file" \
    "/bin/ls /nonexistent_path_xyz 2> $TMPDIR/err.txt" \
    "$TMPDIR/err.txt"

# dup >&2 (stdout goes to stderr, so stdout is empty)
assert_stdout "dup >& sends stdout to stderr" \
    "/bin/echo gone >&2" \
    ""

echo ""
echo "=== Here String Tests ==="

assert_stdout "here string <<<" \
    "cat <<< hello_here" \
    "hello_here"

assert_stdout "here string <<< with var-like word" \
    "cat <<< testing123" \
    "testing123"

echo ""
echo "=== Pipeline + Redirect Tests ==="

assert_file "pipeline last stage redirect >" \
    "/bin/echo piped | cat > $TMPDIR/pipe_out.txt" \
    "$TMPDIR/pipe_out.txt" \
    "piped"

assert_stdout "pipeline with stdin redirect" \
    "cat < $TMPDIR/in.txt | cat" \
    "from_file"

echo ""
echo "=== Results: $PASS passed, $FAIL failed ==="
[ "$FAIL" -eq 0 ]
