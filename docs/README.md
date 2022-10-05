# TeekValidator
- Check a log / character file against a series of regular expressions
- Fails if any matches fail or exclusions exist
- Portable
- Thoroughly tested
- Incredibly performant, optimized for massive datasets
    - Data is buffered once and referred to with `std::string_view`
- Provides comprehensive output
    - Total data read
    - Time taken for each portion of validation
    - Failures
        - Shows which patterns failed against which lines
    - Colorful!

## Usage
```
TeekValidator LOG_FILE REGEXES_FILE
```

## Example Output
![Pass](https://i.imgur.com/uOL91Qt.png)
![Fail](https://i.imgur.com/OGymPZc.png)