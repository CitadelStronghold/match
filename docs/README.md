# TeekValidator
- Check a log / character file against a series of regular expressions
- Fails if any matches fail or exclusions exist
- Incredibly performant, optimized for massive datasets
- Portable
- Thoroughly tested
- Provides comprehensive output
    - Total data read
    - Time taken for each portion of validation
    - Check failures
        - Shows which patterns failed against which lines

## Usage
```
TeekValidator LOG_FILE REGEXES_FILE
```