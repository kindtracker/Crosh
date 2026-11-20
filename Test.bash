# This is AI-Generated.

# Each test documents the functionality it requires.
#
# Test categories:
#   1. Basic commands
#   2. Arguments
#   3. Quoting
#   4. Escaping
#   5. Variables
#   6. Operators
#   7. Redirections
#   8. Comments
#   9. Newlines
#  10. Command combinations
#  11. Conditionals
#  12. Loops
#  13. Functions
#  14. Case statements
#  15. Advanced Bash syntax
#
# ============================================================

# ============================================================
# 1. BASIC COMMANDS
# ============================================================

# Needed functions:
#   Tokenizer:Tokenize
#   Tokenizer:ReadIdent
#   Parser:Parse
#   Parser:ParseCommand
#   LuaGenerator:Generate
#
# Output:
#   Meow
echo Meow

# Needed functions:
#   Parser:ParseCommand
#   LuaGenerator:Generate
#
# Output:
#   Hello World
echo Hello World

# Needed functions:
#   Parser:ParseCommand
#   LuaGenerator:Generate
#
# Output:
#   123
echo 123

# Needed functions:
#   Parser:ParseCommand
#   LuaGenerator:Generate
#
# Output:
#   (empty line)
echo

# ============================================================
# 2. ARGUMENTS
# ============================================================

# Needed functions:
#   Tokenizer:ReadIdent
#   Parser:ParseCommand
#   LuaGenerator:Generate
#
# Output:
#   One Two Three
echo One Two Three

# Needed functions:
#   Parser:ParseCommand
#   LuaGenerator:Generate
#
# Output:
#   -n Hello
echo -n Hello

# Needed functions:
#   Parser:ParseCommand
#   LuaGenerator:Generate
#
# Output:
#   Hello
printf '%s\n' Hello

# ============================================================
# 3. SINGLE QUOTES
# ============================================================

# Needed functions:
#   Tokenizer:ReadQuoted
#   Parser:ParseCommand
#   LuaGenerator:Generate
#
# Output:
#   Hello World
echo 'Hello World'

# Needed functions:
#   Tokenizer:ReadQuoted
#   LuaGenerator:Generate
#
# Output:
#   $HOME
echo '$HOME'

# Needed functions:
#   Tokenizer:ReadQuoted
#   LuaGenerator:Generate
#
# Output:
#   Hello
echo 'Hello'

# ============================================================
# 4. DOUBLE QUOTES
# ============================================================

# Needed functions:
#   Tokenizer:ReadQuoted
#   Parser:ParseCommand
#   LuaGenerator:Generate
#
# Output:
#   Hello World
echo "Hello World"

# Needed functions:
#   Tokenizer:ReadQuoted
#   LuaGenerator:Generate
#
# Output:
#   Hello $USER
# Note: Requires variable expansion.
echo "Hello $USER"

# Needed functions:
#   Tokenizer:ReadQuoted
#   LuaGenerator:Generate
#
# Output:
#   Hello
echo "Hello"

# ============================================================
# 5. ESCAPING
# ============================================================

# Needed functions:
#   Tokenizer:ReadIdent
#   Tokenizer:ReadQuoted
#   LuaGenerator:Generate
#
# Output:
#   Hello World
echo Hello\ World

# Needed functions:
#   Tokenizer:ReadIdent
#   LuaGenerator:Generate
#
# Output:
#   $HOME
echo \$HOME

# Needed functions:
#   Tokenizer:ReadQuoted
#   LuaGenerator:Generate
#
# Output:
#   Hello "World"
echo "Hello \"World\""

# ============================================================
# 6. VARIABLES
# ============================================================

# Needed functions:
#   Tokenizer:ReadIdent
#   Parser:ParseCommand
#   LuaGenerator:Generate
#
# Output:
#   (value of USER)
echo $USER

# Needed functions:
#   Tokenizer:ReadIdent
#   LuaGenerator:Generate
#
# Output:
#   (value of HOME)
echo "$HOME"

# Needed functions:
#   Tokenizer:ReadIdent
#   LuaGenerator:Generate
#
# Output:
#   (value of USER) (value of HOME)
echo $USER $HOME

# Needed functions:
#   Tokenizer:ReadIdent
#   LuaGenerator:Generate
#
# Output:
#   (value of USER)
printf '%s\n' "${USER}"

# Needed functions:
#   Tokenizer:ReadIdent
#   LuaGenerator:Generate
#
# Output:
#   (value of USER)
echo "${USER}"

# ============================================================
# 7. OPERATORS
# ============================================================

# Needed functions:
#   Tokenizer:ReadOperator
#   Parser:Parse
#   Parser:ParseStatement
#   LuaGenerator:Generate
#
# Output:
#   First
#   Second
echo First
echo Second

# Needed functions:
#   Tokenizer:ReadOperator
#   Parser:Parse
#   LuaGenerator:Generate
#
# Output:
#   First
#   Second
echo First && echo Second

# Needed functions:
#   Tokenizer:ReadOperator
#   Parser:Parse
#   LuaGenerator:Generate
#
# Output:
#   First
#   Second
echo First || echo Second

# Needed functions:
#   Tokenizer:ReadOperator
#   Parser:Parse
#   LuaGenerator:Generate
#
# Output:
#   First
echo First &

# ============================================================
# 8. REDIRECTIONS
# ============================================================

# Needed functions:
#   Tokenizer:ReadOperator
#   Parser:ParseCommand
#   LuaGenerator:Generate
#
# Output:
#   File: output.txt
echo Hello >output.txt

# Needed functions:
#   Tokenizer:ReadOperator
#   Parser:ParseCommand
#   LuaGenerator:Generate
#
# Output:
#   File: output.txt (append)
echo Hello >>output.txt

# Needed functions:
#   Tokenizer:ReadOperator
#   Parser:ParseCommand
#   LuaGenerator:Generate
#
# Output:
#   File: input.txt (stdin)
cat <input.txt

# Needed functions:
#   Tokenizer:ReadOperator
#   Parser:ParseCommand
#   LuaGenerator:Generate
#
# Output:
#   stdout redirected to stderr
echo Hello >&2

# Needed functions:
#   Tokenizer:ReadOperator
#   Parser:ParseCommand
#   LuaGenerator:Generate
#
# Output:
#   stderr redirected to stdout
echo Hello 2>&1

# Needed functions:
#   Tokenizer:ReadOperator
#   Parser:ParseCommand
#   LuaGenerator:Generate
#
# Output:
#   stderr redirected to error.log
echo Hello 2>error.log

# Needed functions:
#   Tokenizer:ReadOperator
#   Parser:ParseCommand
#   LuaGenerator:Generate
#
# Output:
#   stdout and stderr redirected to output.log
echo Hello &>output.log

# Needed functions:
#   Tokenizer:ReadOperator
#   Parser:ParseCommand
#   LuaGenerator:Generate
#
# Output:
#   Here-document content
cat <<EOF
Hello
World
EOF

# Needed functions:
#   Tokenizer:ReadOperator
#   Parser:ParseCommand
#   LuaGenerator:Generate
#
# Output:
#   Here-string content
cat <<<"Hello World"

# ============================================================
# 9. COMMENTS
# ============================================================

# Needed functions:
#   Tokenizer:ReadComment
#
# Output:
#   Hello
echo Hello # This is a comment

# Needed functions:
#   Tokenizer:ReadComment
#
# Output:
#   Hello
#   World
echo Hello
# Another comment
echo World

# Needed functions:
#   Tokenizer:ReadComment
#
# Output:
#   (empty)
# Comment only
# Nothing else

# ============================================================
# 10. NEWLINES
# ============================================================

# Needed functions:
#   Tokenizer:ReadNewline
#   Parser:Parse
#   LuaGenerator:Generate
#
# Output:
#   First
#   Second
echo First
echo Second

# Needed functions:
#   Tokenizer:ReadNewline
#   Parser:Parse
#
# Output:
#   First
#   Second
echo First

echo Second

# ============================================================
# 11. IF STATEMENTS
# ============================================================

# Needed functions:
#   Tokenizer:ReadIdent
#   Parser:ParseStatement
#   Parser:ParseIf
#   LuaGenerator:GenerateIf
#
# Output:
#   Yes
if true; then
  echo Yes
fi

# Needed functions:
#   Parser:ParseIf
#   LuaGenerator:GenerateIf
#
# Output:
#   No
if false; then
  echo Yes
else
  echo No
fi

# Needed functions:
#   Parser:ParseIf
#   LuaGenerator:GenerateIf
#
# Output:
#   Yes
if true; then
  echo Yes
elif false; then
  echo Maybe
else
  echo No
fi

# ============================================================
# 12. WHILE LOOPS
# ============================================================

# Needed functions:
#   Parser:ParseWhile
#   LuaGenerator:GenerateWhile
#
# Output:
#   Repeated three times
Counter=0

while [ "$Counter" -lt 3 ]; do
  echo "$Counter"
  Counter=$((Counter + 1))
done

# ============================================================
# 13. FOR LOOPS
# ============================================================

# Needed functions:
#   Parser:ParseFor
#   LuaGenerator:GenerateFor
#
# Output:
#   One
#   Two
#   Three
for Value in One Two Three; do
  echo "$Value"
done

# Needed functions:
#   Parser:ParseFor
#   LuaGenerator:GenerateFor
#
# Output:
#   1
#   2
#   3
for Value in 1 2 3; do
  echo "$Value"
done

# ============================================================
# 14. FUNCTIONS
# ============================================================

# Needed functions:
#   Parser:ParseFunction
#   LuaGenerator:GenerateFunction
#
# Output:
#   Meow
function Meow() {
  echo Meow
}

Meow

# Needed functions:
#   Parser:ParseFunction
#   LuaGenerator:GenerateFunction
#
# Output:
#   Hello
Hello() {
  echo Hello
}

Hello

# ============================================================
# 15. CASE STATEMENTS
# ============================================================

# Needed functions:
#   Parser:ParseCase
#   LuaGenerator:GenerateCase
#
# Output:
#   Two
case 2 in
1)
  echo One
  ;;
2)
  echo Two
  ;;
*)
  echo Other
  ;;
esac

# ============================================================
# 16. GROUPING
# ============================================================

# Needed functions:
#   Tokenizer:ReadOperator
#   Parser:ParseGroup
#   LuaGenerator:GenerateGroup
#
# Output:
#   Hello
{
  echo Hello
}

# Needed functions:
#   Parser:ParseGroup
#   LuaGenerator:GenerateGroup
#
# Output:
#   Hello
(
  echo Hello
)

# ============================================================
# 17. COMMAND SUBSTITUTION
# ============================================================

# Needed functions:
#   Tokenizer:ReadIdent
#   Parser:ParseCommandSubstitution
#   LuaGenerator:GenerateCommandSubstitution
#
# Output:
#   Hello
echo "$(echo Hello)"

# ============================================================
# 18. ARITHMETIC
# ============================================================

# Needed functions:
#   Tokenizer:ReadIdent
#   Parser:ParseArithmetic
#   LuaGenerator:GenerateArithmetic
#
# Output:
#   3
echo $((1 + 2))

# Needed functions:
#   Parser:ParseArithmetic
#   LuaGenerator:GenerateArithmetic
#
# Output:
#   6
echo $((2 * 3))

# ============================================================
# 19. EXIT STATUS
# ============================================================

# Needed functions:
#   Parser:ParseCommand
#   LuaGenerator:Generate
#
# Output:
#   0
true
echo $?

# Needed functions:
#   Parser:ParseCommand
#   LuaGenerator:Generate
#
# Output:
#   1
false
echo $?

# ============================================================
# 20. BUILTIN COMMANDS
# ============================================================

# Needed functions:
#   Parser:ParseCommand
#   LuaGenerator:Generate
#
# Output:
#   (current directory)
pwd

# Needed functions:
#   Parser:ParseCommand
#   LuaGenerator:Generate
#
# Output:
#   (environment variable value)
export CROSH_TEST=Hello
echo "$CROSH_TEST"

# Needed functions:
#   Parser:ParseCommand
#   LuaGenerator:Generate
#
# Output:
#   Hello
cd /tmp
echo Hello

# ============================================================
# END OF TEST SUITE
# ============================================================

echo "Crosh test suite finished."
