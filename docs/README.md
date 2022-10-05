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

## Usage
```
TeekValidator LOG_FILE REGEXES_FILE
```