# Bash Syntax Reference

> [!NOTE]
> This is AI-Generated

A syntax reference for **Bash 5.x**, intended for use when implementing a Bash parser or Bash-to-Lua transpiler.

> **Scope:** This document covers Bash language syntax, including command grammar, operators, quoting, expansions, redirections, compound commands, functions, arrays, and other language constructs. It does not attempt to document every builtin, external command, or runtime behavior.

## Table of Contents

1. [Basic Syntax](#1-basic-syntax)
2. [Tokens and Metacharacters](#2-tokens-and-metacharacters)
3. [Comments](#3-comments)
4. [Quoting and Escaping](#4-quoting-and-escaping)
5. [Command Separators and Operators](#5-command-separators-and-operators)
6. [Simple Commands](#6-simple-commands)
7. [Pipelines](#7-pipelines)
8. [Lists and Conditional Execution](#8-lists-and-conditional-execution)
9. [Redirections](#9-redirections)
10. [Variable Assignments](#10-variable-assignments)
11. [Parameter Expansion](#11-parameter-expansion)
12. [Command Substitution](#12-command-substitution)
13. [Arithmetic Expansion](#13-arithmetic-expansion)
14. [Brace Expansion](#14-brace-expansion)
15. [Tilde Expansion](#15-tilde-expansion)
16. [Word Splitting](#16-word-splitting)
17. [Filename Expansion (Globbing)](#17-filename-expansion-globbing)
18. [Process Substitution](#18-process-substitution)
19. [Conditional Commands](#19-conditional-commands)
20. [Loops](#20-loops)
21. [Case Statements](#21-case-statements)
22. [Functions](#22-functions)
23. [Arrays](#23-arrays)
24. [Arithmetic Commands](#24-arithmetic-commands)
25. [Grouping Commands](#25-grouping-commands)
26. [Coprocesses](#26-coprocesses)
27. [Here Documents](#27-here-documents)
28. [Here Strings](#28-here-strings)
29. [Shell Parameters](#29-shell-parameters)
30. [Reserved Words](#30-reserved-words)
31. [Shell Options and Extended Syntax](#31-shell-options-and-extended-syntax)
32. [Expansion Order](#32-expansion-order)
33. [Parser Implementation Notes](#33-parser-implementation-notes)

---

## 1. Basic Syntax

A Bash program consists of commands and command lists.

A command can be:

- A simple command.
- A pipeline.
- A list of commands.
- A compound command.
- A function definition.

Example:

```bash
#!/usr/bin/env bash

NAME="World"

if [[ -n "$NAME" ]]; then
    echo "Hello, $NAME!"
fi
```

Bash is not line-oriented in the sense that every newline ends a command. Newlines can also occur inside compound commands, quoted strings, and continued lines.

Whitespace generally separates words, but its meaning depends on context.

## 2. Tokens and Metacharacters

### 2.1 Metacharacters

These characters have special meaning when unquoted:

```text
|  &  ;  (  )  <  >  space  tab  newline
```

### 2.2 Control operators

```text
||  &&  &  ;  ;;  ;&  ;;&  |  |&  newline
```

### 2.3 Other special tokens

```text
!  [[  ]]  ((  ))  {  }
```

Some of these are recognized as operators only in specific grammatical contexts.

### 2.4 Longest-match tokenization

A lexer should generally recognize the longest applicable operator.

For example:

```bash
command 2>&1
```

The lexer must recognize `>&` as a redirection operator rather than treating `>` and `&` as unrelated tokens.

Likewise:

```bash
command &>> output.txt
```

Must recognize `&>>` as one operator.

Quoted characters must not become operators.

For example:

```bash
echo "hello > world"
```

The `>` is part of a word, not a redirection.

---

## 3. Comments

A comment begins with an unquoted `#` when it occurs where a new word can begin.

```bash
# This is a comment

echo hello # This is also a comment
```

A `#` inside a word is not necessarily a comment:

```bash
echo hello#world
```

The following is a shebang:

```bash
#!/bin/bash
```

A shebang is interpreted by the operating system when executing a script directly. It is not a general Bash command.

---

## 4. Quoting and Escaping

Bash provides four main quoting mechanisms.

### 4.1 Backslash escaping

A backslash preserves the literal meaning of the next character.

```bash
echo hello\ world
```

Produces one argument:

```text
hello world
```

A backslash followed by a newline is a line continuation:

```bash
echo hello \
     world
```

The backslash-newline pair is removed.

### 4.2 Single quotes

Single quotes preserve the literal value of every enclosed character.

```bash
echo 'Hello $NAME'
```

The variable is not expanded.

Single quotes cannot directly contain another single quote.

### 4.3 Double quotes

Double quotes preserve most characters literally while allowing certain expansions.

```bash
echo "Hello $NAME"
```

Inside double quotes:

- `$` still introduces expansions.
- Backticks still introduce command substitution.
- `"` ends the quoted section.
- `\` has special behavior before `$`, backtick, `"`, `\`, and newline.
- Other characters are generally preserved literally.

Example:

```bash
echo "Path: $HOME"
echo "Literal dollar: \$"
```

Double quotes prevent ordinary word splitting and filename expansion of the quoted result.

### 4.4 ANSI-C quoting

Syntax:

```bash
$'string'
```

This interprets escape sequences.

Examples:

```bash
echo $'Hello\nWorld'
echo $'Tab:\tText'
echo $'\x41'
```

Common escapes:

| Escape | Meaning |
|---|---|
| `\a` | Alert |
| `\b` | Backspace |
| `\e` | Escape |
| `\f` | Form feed |
| `\n` | Newline |
| `\r` | Carriage return |
| `\t` | Horizontal tab |
| `\v` | Vertical tab |
| `\\` | Backslash |
| `\'` | Single quote |
| `\"` | Double quote |
| `\nnn` | Octal character |
| `\xHH` | Hexadecimal character |
| `\uHHHH` | Unicode character |
| `\UHHHHHHHH` | Unicode character |

### 4.5 Locale-specific translation quoting

Syntax:

```bash
$"string"
```

This marks a string for locale-specific translation when the relevant localization facilities are enabled.

Example:

```bash
echo $"Hello"
```

---

## 5. Command Separators and Operators

### 5.1 Semicolon

```bash
command1; command2
```

Runs the first command, then the second.

### 5.2 Newline

```bash
command1
command2
```

A newline normally separates commands, just like a semicolon.

### 5.3 Background operator

```bash
command &
```

Runs a command asynchronously.

### 5.4 AND operator

```bash
command1 && command2
```

Runs `command2` only if `command1` succeeds (exit status zero).

### 5.5 OR operator

```bash
command1 || command2
```

Runs `command2` only if `command1` fails (nonzero exit status).

### 5.6 Negation

```bash
! command
```

Inverts the command's exit status.

### 5.7 Line continuation

```bash
command \
    argument
```

A backslash-newline pair is removed before normal parsing.

---

## 6. Simple Commands

A simple command consists of words, optional variable assignments, and redirections.

```bash
NAME=value command argument
```

Examples:

```bash
echo hello
ls -la
NAME=hello
NAME=hello command
command arg1 arg2
command > output.txt
```

A simple command can contain multiple assignments and redirections:

```bash
A=one B=two command arg1 > output.txt 2>&1
```

Assignments before the command name are assignment words. Their interpretation differs from ordinary arguments.

Assignments after the command name are ordinary arguments unless the command itself interprets them.

---

## 7. Pipelines

A pipeline connects the output of one command to the input of another.

### 7.1 Standard pipeline

```bash
command1 | command2
```

The standard output of `command1` becomes the standard input of `command2`.

### 7.2 Multiple commands

```bash
command1 | command2 | command3
```

### 7.3 Pipe stderr too

```bash
command1 |& command2
```

Connects both standard output and standard error to the next command's input.

### 7.4 Negated pipeline

```bash
! command1 | command2
```

Negates the pipeline's resulting status.

### 7.5 Pipeline with time

```bash
time command1 | command2
```

Measures the execution time of the pipeline.

### 7.6 Pipeline subshell behavior

By default, Bash commonly executes commands in a pipeline in subshell environments. The `lastpipe` shell option can change the behavior of the last pipeline command under specific conditions.

---

## 8. Lists and Conditional Execution

### 8.1 AND lists

```bash
command1 && command2 && command3
```

Commands execute from left to right while preceding commands succeed.

### 8.2 OR lists

```bash
command1 || command2 || command3
```

Commands execute from left to right while preceding commands fail.

### 8.3 Combined lists

```bash
command1 && command2 || command3
```

`&&` and `||` have equal precedence and are evaluated left-associatively.

Use grouping when the intended meaning needs to be explicit:

```bash
(command1 && command2) || command3
```

### 8.4 Background lists

```bash
command1 && command2 &
```

Runs the AND list asynchronously.

---

## 9. Redirections

Redirections change where a command reads input or sends output.

File descriptors commonly used:

| Descriptor | Meaning |
|---|---|
| `0` | Standard input |
| `1` | Standard output |
| `2` | Standard error |

### 9.1 Output redirection

```bash
command > file
```

Redirects stdout, replacing the file's previous contents.

### 9.2 Append output

```bash
command >> file
```

Appends stdout to a file.

### 9.3 Input redirection

```bash
command < file
```

Reads stdin from a file.

### 9.4 Redirect stderr

```bash
command 2> errors.txt
```

### 9.5 Append stderr

```bash
command 2>> errors.txt
```

### 9.6 Redirect stdout and stderr

```bash
command &> output.txt
command >& output.txt
```

### 9.7 Append stdout and stderr

```bash
command &>> output.txt
```

### 9.8 Duplicate file descriptors

```bash
command 2>&1
```

Redirects stderr to the current destination of stdout.

```bash
command 1>&2
```

Redirects stdout to stderr.

### 9.9 Close a file descriptor

```bash
command 2>&-
```

### 9.10 Redirect a specific descriptor

```bash
command 3> file
command 4< file
```

### 9.11 Read/write redirection

```bash
command <> file
```

Opens a file for both reading and writing.

### 9.12 Clobber override

```bash
command >| file
```

Forces output redirection even when `noclobber` is enabled.

### 9.13 Here document

```bash
command <<EOF
Hello
World
EOF
```

Feeds text to the command's standard input.

### 9.14 Indented here document

```bash
command <<-EOF
    Hello
    World
EOF
```

Removes leading tab characters from the document lines and delimiter.

### 9.15 Here string

```bash
command <<< "Hello World"
```

Feeds a string to standard input, followed by a newline.

### 9.16 Redirection ordering

Redirections are processed from left to right.

These are different:

```bash
command > output.txt 2>&1
command 2>&1 > output.txt
```

In the first example, both stdout and stderr go to the file.

In the second example, stderr is duplicated from stdout before stdout is redirected.

### 9.17 Redirections without commands

```bash
> output.txt
< input.txt
```

A redirection can occur without a command name.

---

## 10. Variable Assignments

### 10.1 Basic assignment

```bash
NAME=value
```

There must be no spaces around `=`.

Incorrect:

```bash
NAME = value
```

### 10.2 Expansion in assignments

```bash
NAME="$USER"
COUNT=$((10 + 5))
```

### 10.3 Multiple assignments

```bash
A=one B=two
```

### 10.4 Export

```bash
export NAME=value
export NAME
```

### 10.5 Unset

```bash
unset NAME
```

`export` and `unset` are builtins, not special assignment operators.

---

## 11. Parameter Expansion

Parameter expansion uses `$` or `${...}`.

### 11.1 Basic expansion

```bash
$NAME
${NAME}
```

Braces delimit the parameter name.

```bash
echo "${NAME}_suffix"
```

### 11.2 Default value

```bash
${NAME:-default}
```

Uses `default` if `NAME` is unset or null.

### 11.3 Default assignment

```bash
${NAME:=default}
```

Assigns `default` to `NAME` if it is unset or null.

### 11.4 Error if unset or null

```bash
${NAME:?error message}
```

Produces an error if `NAME` is unset or null.

### 11.5 Alternate value

```bash
${NAME:+alternate}
```

Uses `alternate` if `NAME` is set and non-null.

### 11.6 Unset-only variants

```bash
${NAME-default}
${NAME=default}
${NAME?error}
${NAME+alternate}
```

Unlike the `:` forms, these test whether the variable is unset, not whether it is empty.

### 11.7 String length

```bash
${#NAME}
```

### 11.8 Substring expansion

```bash
${NAME:offset}
${NAME:offset:length}
```

Example:

```bash
TEXT=HelloWorld
echo "${TEXT:0:5}"
```

### 11.9 Remove shortest prefix

```bash
${NAME#pattern}
```

### 11.10 Remove longest prefix

```bash
${NAME##pattern}
```

### 11.11 Remove shortest suffix

```bash
${NAME%pattern}
```

### 11.12 Remove longest suffix

```bash
${NAME%%pattern}
```

### 11.13 Replace first matching pattern

```bash
${NAME/pattern/replacement}
```

### 11.14 Replace all matching patterns

```bash
${NAME//pattern/replacement}
```

### 11.15 Replace prefix match

```bash
${NAME/#pattern/replacement}
```

### 11.16 Replace suffix match

```bash
${NAME/%pattern/replacement}
```

### 11.17 Case modification

```bash
${NAME^}
${NAME^^}
${NAME,}
${NAME,,}
```

These modify the first matching character or all matching characters, depending on the operator.

### 11.18 Indirect expansion

```bash
${!NAME}
```

Expands the value of the variable whose name is stored in `NAME`.

### 11.19 Indirect prefix expansion

```bash
${!PREFIX*}
${!PREFIX@}
```

Expands variable names matching a prefix.

### 11.20 Parameter transformation

Bash also supports transformations using:

```bash
${NAME@operator}
```

Examples include:

- `Q`: Quote the value for reuse as shell input.
- `E`: Interpret escape sequences.
- `P`: Expand the value as a prompt string.
- `A`: Produce an assignment-like representation.
- `a`: Produce variable attributes.
- `U`: Convert to uppercase.
- `u`: Capitalize the first character.
- `L`: Convert to lowercase.

---

## 12. Command Substitution

Command substitution replaces a construct with the output of a command.

### 12.1 Modern syntax

```bash
$(command)
```

Example:

```bash
DATE=$(date)
```

Trailing newline characters are removed from the captured output.

### 12.2 Legacy syntax

```bash
`command`
```

Example:

```bash
DATE=`date`
```

Backticks are harder to nest and should generally be avoided in new scripts.

### 12.3 Nested substitution

```bash
RESULT=$(echo "$(date)")
```

### 12.4 Current-environment command substitution

Bash also supports:

```bash
${ command; }
```

This executes the command in the current execution environment and captures stdout.

### 12.5 REPLY-based command substitution

```bash
${| REPLY=value; }
```

This uses `REPLY` as the substitution result without capturing stdout in the ordinary way.

These last two forms are Bash-specific extensions.

---

## 13. Arithmetic Expansion

Arithmetic expansion uses:

```bash
$((expression))
```

Examples:

```bash
echo $((1 + 2))
echo $((10 * 5))
echo $((10 / 2))
```

### 13.1 Arithmetic operators

| Operator | Meaning |
|---|---|
| `+` | Addition |
| `-` | Subtraction |
| `*` | Multiplication |
| `/` | Division |
| `%` | Remainder |
| `**` | Exponentiation |
| `++` | Increment |
| `--` | Decrement |
| `+=` | Add and assign |
| `-=` | Subtract and assign |
| `*=` | Multiply and assign |
| `/=` | Divide and assign |
| `%=` | Remainder and assign |
| `<<` | Left shift |
| `>>` | Right shift |
| `&` | Bitwise AND |
| `\|` | Bitwise OR |
| `^` | Bitwise XOR |
| `~` | Bitwise NOT |
| `!` | Logical NOT |
| `&&` | Logical AND |
| `\|\|` | Logical OR |
| `==` | Equality |
| `!=` | Inequality |
| `<` | Less than |
| `>` | Greater than |
| `<=` | Less than or equal |
| `>=` | Greater than or equal |
| `,` | Expression separator |
| `?:` | Conditional expression |

Parentheses can group expressions:

```bash
echo $(( (2 + 3) * 4 ))
```

Bash arithmetic uses integer arithmetic.

---

## 14. Brace Expansion

Brace expansion generates multiple words.

### 14.1 Comma-separated alternatives

```bash
echo {one,two,three}
```

Result:

```text
one two three
```

### 14.2 Prefix and suffix

```bash
echo file{1,2,3}.txt
```

### 14.3 Numeric sequences

```bash
echo {1..5}
```

### 14.4 Numeric sequences with steps

```bash
echo {1..10..2}
```

### 14.5 Reverse sequences

```bash
echo {5..1}
```

### 14.6 Alphabetic sequences

```bash
echo {a..z}
```

### 14.7 Nested brace expansion

```bash
echo {a,b}{1,2}
```

Brace expansion is textual and occurs before parameter expansion.

It is not the same as filename globbing.

---

## 15. Tilde Expansion

A leading unquoted tilde can represent a home or directory location.

```bash
~
~/Documents
~username
~+
~-
```

Meanings:

| Syntax | Meaning |
|---|---|
| `~` | Current user's home directory |
| `~/path` | Path inside the home directory |
| `~username` | Specified user's home directory |
| `~+` | Current working directory |
| `~-` | Previous working directory |

Bash also supports directory-stack forms such as:

```bash
~+2
~-1
```

---

## 16. Word Splitting

After certain expansions, Bash may split a word into multiple words using characters in `IFS`.

Example:

```bash
VALUE="one two three"
command $VALUE
```

This can produce three arguments.

Quoted expansion:

```bash
command "$VALUE"
```

Produces one argument.

Word splitting normally applies to unquoted parameter expansions, command substitutions, and arithmetic expansions.

It does not apply to the results of expansions inside double quotes.

---

## 17. Filename Expansion (Globbing)

Filename expansion matches filenames against patterns.

### 17.1 Asterisk

```bash
*.txt
```

Matches zero or more characters.

### 17.2 Question mark

```bash
file?.txt
```

Matches exactly one character.

### 17.3 Character classes

```bash
file[123].txt
```

Matches one character from the set.

### 17.4 Character ranges

```bash
file[a-z].txt
```

Matches a character in the specified range.

### 17.5 Negated character classes

```bash
file[!0-9].txt
```

Matches a character outside the set.

### 17.6 Extended globbing

When `extglob` is enabled:

| Pattern | Meaning |
|---|---|
| `?(pattern)` | Zero or one occurrence |
| `*(pattern)` | Zero or more occurrences |
| `+(pattern)` | One or more occurrences |
| `@(pattern)` | Exactly one alternative |
| `!(pattern)` | Anything except the pattern |

Example:

```bash
shopt -s extglob
echo !(*.txt)
```

### 17.7 Recursive globbing

With `globstar` enabled:

```bash
shopt -s globstar
echo **/*.c
```

`**` can match directories recursively.

### 17.8 Important distinction

A glob pattern is not a regular expression.

For example:

```bash
*.txt
```

is a shell filename pattern, not a regex.

---

## 18. Process Substitution

Process substitution provides a process through a filename-like interface.

### 18.1 Read from a process

```bash
command <(other_command)
```

### 18.2 Write to a process

```bash
command >(other_command)
```

Example:

```bash
diff <(command1) <(command2)
```

Process substitution runs the specified command asynchronously and exposes its input or output through a filename-like path.

It is supported on systems that provide an appropriate `/dev/fd` or named-pipe mechanism.

---

## 19. Conditional Commands

Bash has several forms of conditional syntax.

### 19.1 `if`

```bash
if condition; then
    commands
fi
```

### 19.2 `if` / `else`

```bash
if condition; then
    commands
else
    commands
fi
```

### 19.3 `if` / `elif` / `else`

```bash
if condition1; then
    commands
elif condition2; then
    commands
else
    commands
fi
```

### 19.4 `test`

```bash
test expression
```

Equivalent shorthand:

```bash
[ expression ]
```

The spaces around the brackets are required.

### 19.5 Extended test

```bash
[[ expression ]]
```

`[[ ... ]]` is a Bash keyword construct, not an ordinary external command.

It supports:

- String comparisons.
- Numeric comparisons.
- Pattern matching with `==` and `!=`.
- Regular expression matching with `=~`.
- Logical operators `&&`, `||`, and `!`.
- Parentheses for grouping.

Example:

```bash
if [[ "$NAME" == "hello" ]]; then
    echo match
fi
```

### 19.6 Conditional expression operators

Common string operators:

```text
-n STRING
-z STRING
STRING1 == STRING2
STRING1 != STRING2
STRING1 < STRING2
STRING1 > STRING2
```

Common numeric operators:

```text
INTEGER1 -eq INTEGER2
INTEGER1 -ne INTEGER2
INTEGER1 -lt INTEGER2
INTEGER1 -le INTEGER2
INTEGER1 -gt INTEGER2
INTEGER1 -ge INTEGER2
```

Common file tests:

```text
-e FILE    File exists
-f FILE    Regular file
-d FILE    Directory
-r FILE    Readable
-w FILE    Writable
-x FILE    Executable
-s FILE    Nonzero size
-L FILE    Symbolic link
-b FILE    Block device
-c FILE    Character device
-p FILE    Named pipe
-S FILE    Socket
```

Additional tests exist for ownership, permissions, timestamps, and file relationships.

### 19.7 `case`

```bash
case "$VALUE" in
    pattern1)
        commands
        ;;
    pattern2)
        commands
        ;;
    *)
        commands
        ;;
esac
```

Patterns use shell pattern matching.

Case terminators:

| Terminator | Behavior |
|---|---|
| `;;` | Stop processing the case statement |
| `;&` | Execute the next case body without testing its pattern |
| `;;&` | Continue testing subsequent patterns |

---

## 20. Loops

### 20.1 `while`

```bash
while condition; do
    commands
done
```

Repeats while the condition succeeds.

### 20.2 `until`

```bash
until condition; do
    commands
done
```

Repeats until the condition succeeds.

### 20.3 `for`

```bash
for VARIABLE in WORDS; do
    commands
done
```

Example:

```bash
for FILE in *.txt; do
    echo "$FILE"
done
```

### 20.4 C-style `for`

```bash
for (( initialization; condition; increment )); do
    commands
done
```

Example:

```bash
for ((i = 0; i < 10; i++)); do
    echo "$i"
done
```

### 20.5 `select`

```bash
select VARIABLE in WORDS; do
    commands
done
```

Creates a numbered selection menu.

### 20.6 Loop control

```bash
break
continue
```

Both can accept a numeric argument specifying how many nested loops to affect.

---

## 21. Case Statements

The full form:

```bash
case WORD in
    PATTERN1)
        COMMANDS
        ;;
    PATTERN2 | PATTERN3)
        COMMANDS
        ;&
    PATTERN4)
        COMMANDS
        ;;&
    *)
        DEFAULT_COMMANDS
        ;;
esac
```

Patterns may contain shell pattern operators such as `*`, `?`, and bracket expressions.

---

## 22. Functions

### 22.1 Standard function syntax

```bash
function_name() {
    commands
}
```

### 22.2 `function` keyword syntax

```bash
function function_name {
    commands
}
```

Bash also accepts:

```bash
function function_name() {
    commands
}
```

### 22.3 Calling a function

```bash
function_name argument1 argument2
```

### 22.4 Function arguments

Inside a function:

| Parameter | Meaning |
|---|---|
| `$0` | Script or shell name |
| `$1` | First positional parameter |
| `$2` | Second positional parameter |
| `$#` | Number of positional parameters |
| `$@` | All positional parameters |
| `$*` | All positional parameters as a combined value |

### 22.5 Return

```bash
return
return 0
return 1
```

A function's return status is its exit status.

### 22.6 Local variables

```bash
function_name() {
    local VALUE=hello
}
```

`local` is a Bash builtin.

### 22.7 Variable scope

Bash functions normally use dynamically scoped local variables: a function can access local variables belonging to its callers.

---

## 23. Arrays

Bash supports indexed arrays and associative arrays.

### 23.1 Indexed arrays

```bash
ARRAY=(one two three)
```

### 23.2 Indexed assignment

```bash
ARRAY[0]=one
ARRAY[1]=two
```

### 23.3 Read an element

```bash
echo "${ARRAY[0]}"
```

### 23.4 Array length

```bash
echo "${#ARRAY[@]}"
```

### 23.5 All elements

```bash
echo "${ARRAY[@]}"
```

### 23.6 All indices

```bash
echo "${!ARRAY[@]}"
```

### 23.7 Array slices

```bash
echo "${ARRAY[@]:offset:length}"
```

### 23.8 Append elements

```bash
ARRAY+=(four five)
```

### 23.9 Remove an element

```bash
unset 'ARRAY[1]'
```

### 23.10 Associative arrays

```bash
declare -A MAP
MAP[name]=hello
MAP[language]=bash
```

Read an element:

```bash
echo "${MAP[name]}"
```

Read all values:

```bash
echo "${MAP[@]}"
```

Read all keys:

```bash
echo "${!MAP[@]}"
```

Associative arrays require Bash support and are not POSIX shell syntax.

---

## 24. Arithmetic Commands

Arithmetic commands use:

```bash
(( expression ))
```

Example:

```bash
((COUNT++))
((COUNT += 5))
```

The command's exit status is based on whether the arithmetic expression evaluates to a nonzero value.

Arithmetic `for` loops use the same arithmetic language:

```bash
for ((i = 0; i < 10; i++)); do
    echo "$i"
done
```

---

## 25. Grouping Commands

### 25.1 Subshell grouping

```bash
(
    command1
    command2
)
```

Commands execute in a subshell environment.

Changes to ordinary shell variables and the current directory do not persist in the parent shell.

### 25.2 Brace grouping

```bash
{
    command1
    command2
}
```

Commands execute in the current shell environment.

Whitespace after `{` and before `}` is significant, and the closing `}` must appear where a reserved word is allowed.

### 25.3 Group redirection

```bash
{
    command1
    command2
} > output.txt
```

Redirects the grouped commands' output.

---

## 26. Coprocesses

Bash supports asynchronous coprocesses.

Syntax:

```bash
coproc NAME {
    commands
}
```

A simpler form is:

```bash
coproc commands
```

A coprocess runs asynchronously with pipes connected to the shell.

Bash exposes file descriptors and a process ID through variables associated with the coprocess.

Example:

```bash
coproc WORKER {
    echo ready
    read INPUT
    echo "$INPUT"
}
```

Coprocesses are a Bash-specific feature.

---

## 27. Here Documents

A here document supplies a block of text as command input.

### 27.1 Standard form

```bash
command <<EOF
Text
More text
EOF
```

### 27.2 Quoted delimiter

```bash
command <<'EOF'
$HOME is not expanded
EOF
```

Quoting any part of the delimiter prevents the usual parameter, command, and arithmetic expansions in the document body.

### 27.3 Unquoted delimiter

```bash
command <<EOF
Home: $HOME
EOF
```

Expansions are performed.

### 27.4 Tab-stripping form

```bash
command <<-EOF
	Indented text
	EOF
```

Leading tab characters are removed.

### 27.5 Multiple here documents

```bash
command <<FIRST <<SECOND
first document
FIRST
second document
SECOND
```

The order of redirections matters.

---

## 28. Here Strings

Syntax:

```bash
command <<< WORD
```

Example:

```bash
grep hello <<< "hello world"
```

The expanded word is supplied to standard input with a trailing newline.

Unlike a here document, a here string supplies a single expanded word.

---

## 29. Shell Parameters

### 29.1 Positional parameters

```bash
$0
$1
$2
${10}
```

### 29.2 Special parameters

| Parameter | Meaning |
|---|---|
| `$0` | Shell or script name |
| `$1` ... `$N` | Positional parameters |
| `$#` | Number of positional parameters |
| `$@` | All positional parameters |
| `$*` | All positional parameters |
| `$?` | Last command's exit status |
| `$$` | Shell process ID |
| `$!` | Most recent background process ID |
| `$-` | Current shell option flags |
| `$_` | Last argument of the previous command, with context-dependent behavior |

### 29.3 Quoted positional parameters

```bash
"$@"
"$*"
```

These are not interchangeable.

`"$@"` preserves positional parameters as separate words.

`"$*"` combines them into one word using the first character of `IFS` as the separator.

### 29.4 Shift

```bash
shift
shift 2
```

`shift` is a builtin that changes positional parameters.

---

## 30. Reserved Words

Bash recognizes reserved words in specific grammatical positions.

Common reserved words:

```text
!
case
coproc
do
done
elif
else
esac
fi
for
function
if
in
select
then
time
until
while
{
}
[[ 
]]
```

Reserved words are context-sensitive.

For example, `if` is recognized as a reserved word when it occurs where a command can begin:

```bash
if true; then
    echo yes
fi
```

But it can also be ordinary argument text:

```bash
echo if
```

Quoting prevents reserved-word recognition:

```bash
echo "if"
```

---

## 31. Shell Options and Extended Syntax

Some Bash syntax depends on shell options.

### 31.1 `extglob`

Enables extended glob patterns:

```bash
shopt -s extglob
```

### 31.2 `globstar`

Enables recursive `**` filename expansion:

```bash
shopt -s globstar
```

### 31.3 `nullglob`

Unmatched filename patterns expand to nothing:

```bash
shopt -s nullglob
```

### 31.4 `failglob`

Unmatched filename patterns cause an expansion error:

```bash
shopt -s failglob
```

### 31.5 `nocaseglob`

Makes filename expansion case-insensitive:

```bash
shopt -s nocaseglob
```

### 31.6 `lastpipe`

Allows the final command of a pipeline to run in the current shell environment under certain conditions.

### 31.7 `posix`

Enables behavior intended to improve POSIX compatibility:

```bash
set -o posix
```

### 31.8 `pipefail`

Changes pipeline exit-status behavior:

```bash
set -o pipefail
```

A transpiler should decide whether to preserve the source shell's option settings or implement a fixed set of Bash semantics.

---

## 32. Expansion Order

Expansion order is particularly important when implementing a Bash-compatible parser and runtime.

Bash generally performs these expansions in this order:

1. Brace expansion.
2. Tilde expansion.
3. Parameter expansion.
4. Arithmetic expansion.
5. Command substitution.
6. Process substitution, where supported (performed alongside the relevant expansion stages).
7. Word splitting.
8. Filename expansion.
9. Quote removal.

Parameter expansion, arithmetic expansion, and command substitution are performed in a left-to-right manner within the relevant stage.

### Example

```bash
FILES="*.txt"
echo $FILES
```

The unquoted expansion can undergo word splitting and filename expansion.

Compare:

```bash
echo "$FILES"
```

The quoted expansion does not undergo ordinary word splitting or filename expansion.

### Important parser distinction

Expansion is not the same as parsing.

For example:

```bash
COMMAND="echo hello"
$COMMAND
```

The expanded result does not simply get reparsed as a fresh Bash command line. It undergoes the applicable expansion stages and becomes command words.

This distinction is essential for a correct transpiler.

---

## 33. Parser Implementation Notes

This section is especially relevant when translating Bash to Lua.

### 33.1 Do not parse Bash with string splitting

This is not sufficient:

```c
strtok(Input, " ");
```

Consider:

```bash
echo "hello world"
echo 'hello > world'
echo "$(echo hello)"
echo file{1,2}.txt
```

Spaces and operators can occur inside quoted strings and nested constructs.

A lexer needs to understand quoting and nesting.

### 33.2 Preserve word structure

A Bash word is not necessarily one plain string.

For example:

```bash
"hello $NAME"
```

Contains literal text and a parameter expansion.

Likewise:

```bash
file"$EXT".txt
```

Combines multiple word fragments.

A useful AST representation is:

```text
Word
├── Literal("file")
├── ParameterExpansion("EXT")
└── Literal(".txt")
```

The quote context should also be preserved because it affects later expansion behavior.

### 33.3 Suggested AST node types

```text
Program
CommandList
Pipeline
AndOrList
SimpleCommand
Assignment
Word
WordPart
Redirection
IfStatement
WhileLoop
UntilLoop
ForLoop
ArithmeticForLoop
CaseStatement
FunctionDefinition
Subshell
BraceGroup
ArithmeticCommand
ConditionalCommand
CommandSubstitution
ProcessSubstitution
HereDocument
HereString
```

### 33.4 Suggested word-part types

```text
Literal
SingleQuoted
DoubleQuoted
EscapedCharacter
ParameterExpansion
CommandSubstitution
ArithmeticExpansion
ProcessSubstitution
TildeExpansion
BraceExpansion
```

You may choose to represent quotes as metadata on word parts instead of separate node types.

### 33.5 Parse before expanding

Do not expand variables while lexing or parsing.

For example:

```bash
VALUE='hello > output.txt'
echo $VALUE
```

The `>` comes from an expansion. It must not suddenly become a redirection operator.

The parser should recognize operators from the original source syntax, not from expanded text.

### 33.6 Redirections belong to commands

A useful representation:

```text
SimpleCommand
├── Words
│   ├── "echo"
│   └── "hello"
└── Redirections
    └── OutputRedirect
        ├── FileDescriptor: 1
        └── Target: "output.txt"
```

Remember that redirections can appear in different positions:

```bash
command > output.txt
> output.txt command
command arg > output.txt
```

### 33.7 Recommended two-pass architecture

**Pass 1 — Lexer and parser**

- Tokenize input.
- Track quoting and escaping.
- Recognize operators.
- Parse nested command substitutions.
- Parse compound commands.
- Build an AST.
- Preserve source order and quote context.

**Pass 2 — Lua code generation**

- Walk the AST.
- Generate Lua expressions and statements.
- Emit calls to a shell-runtime library for shell-specific behavior.
- Generate control flow.
- Preserve command and pipeline semantics.

You do not need a separate semantic-analysis pass initially. You can add one later if the implementation needs validation, optimization, or more complex scope handling.

### 33.8 Runtime features

Some Bash constructs cannot be represented by simple Lua syntax alone.

A runtime library will likely need to implement:

- Command execution.
- Environment variables.
- Shell-local variables.
- Positional parameters.
- Exit statuses.
- Redirections.
- Pipes.
- Background execution.
- Filename expansion.
- Word splitting.
- Command substitution.
- Process substitution.
- Shell functions.
- Working-directory changes.
- File tests.

For example, generated Lua might look like:

```lua
Runtime.execute({
    command = "echo",
    arguments = { "hello" },
    stdout = Runtime.redirect("output.txt", "write")
})
```

This is illustrative pseudocode: the exact runtime API is up to your implementation.

---

## References

- [GNU Bash Reference Manual](https://www.gnu.org/software/bash/manual/bash.html) — primary language and behavior reference.
- [Bash manual: Shell Syntax](https://www.gnu.org/software/bash/manual/html_node/Shell-Syntax.html)
- [Bash manual: Shell Commands](https://www.gnu.org/software/bash/manual/html_node/Shell-Commands.html)
- [Bash manual: Shell Expansions](https://www.gnu.org/software/bash/manual/html_node/Shell-Expansions.html)
- [Bash manual: Redirections](https://www.gnu.org/software/bash/manual/html_node/Redirections.html)

**Compatibility note:** Bash syntax and behavior vary with Bash version, shell options, and execution environment. This document targets modern Bash 5.x and is a practical syntax reference, not a formal replacement for the complete GNU Bash Reference Manual.
