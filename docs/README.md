# Match

![Tests](https://github.com/GeodeGames/match/actions/workflows/Test.yml/badge.svg)

**Introducing Match: The Simple Log Validation Tool**

Match is the indispensable tool designed to simplify and automate log validation tasks. Tailored specifically for developers, Match allows you to effortlessly verify that your tests generate the expected log output, ensuring the smooth functioning of your continuous integration (CI) pipeline. Let's explore how Match can elevate your development workflow:

- Effortless Log Validation
    - Match seamlessly checks log files against a series of regular expressions, eliminating the need for tedious manual verification. Say goodbye to time-consuming and error-prone processes!
- Fail-Safe Verification
    - Match ensures early detection of problems by flagging any matches that fail or exclusions that exist. Trustworthy results are just a step away.
- Portability
    - Match is designed to be highly portable, effortlessly integrating into various environments and platforms. Whether you're working on different operating systems or deployment setups, Match has got you covered.
- Rock-Solid Reliability
    - Trust is paramount, and Match delivers. Rigorously tested for stability and robustness, it leaves no room for errors or surprises in your code. Rely on Match to ensure the integrity of your log data.
- Lightning-Fast Performance
    - Experience the unmatched speed of Match, fine-tuned to handle massive datasets with lightning efficiency. Through smart data buffering and optimal utilization of `std::string_view`, Match delivers unparalleled performance gains, empowering you to process logs swiftly and cost effectively.
- Comprehensive Output
    - Get an in-depth understanding of your data validation process with Match's comprehensive output. Track crucial metrics, including total data read, time taken for each validation step, and detailed failure information.
    - Match's output is not only informative but also visually enhanced with vibrant colors, making analysis a breeze.
- Multithreading Automation
    - Boost your CI times with Match's automatic multithreading capability.

| OS       | Compiler |
| -------- | -------- |
| Linux    | GCC      |
| Windows  | MSVC     |

## Usage
```
match <LOG_FILE> <REGEX_FILE>
```

Match is a command-line tool designed to analyze log files by matching them against regular expressions.

The tool requires two input files: a log file and a text file containing regex patterns. The parser operates in two modes: inclusive regex and exclusive regex, which can be determined by the presence of an empty line in the regex file.

Lines in the regex file starting with the '#' character are considered as comments and are ignored by the parser.

### [Examples](https://github.com/Eshnek/match/tree/main/tests)

## Example Output
![Pass](https://i.imgur.com/uOL91Qt.png)
![Fail](https://i.imgur.com/OGymPZc.png)

## Github Actions Usage
