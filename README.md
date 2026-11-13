# LEX Tool Using C++

## Project Overview
An educational LEX-inspired lexical analyzer mini-project developed using C++17. A custom rule generator reads lexical rules and generates C++ scanner source. A Python standard-library server connects the scanner to a browser-based interface.

## Features
- Keyword and identifier recognition
- Integer and decimal floating-point literals
- String and character literals
- Single-line and multi-line comments
- Arithmetic, relational, logical and assignment operators
- Compound assignment and increment/decrement operators
- Punctuation recognition
- Longest-match token recognition
- Lexical error reporting with line and column information
- Browser-based interface

## Technologies
- C++17
- Python 3 standard library
- HTML, CSS and JavaScript
- PowerShell for Windows automation

## Project Structure
- `src/` - C++ rule generator and scanner
- `tests/` - lexical rules and test scripts
- `web/` - browser interface
- `server.py` - Python backend
- `start.bat` - Windows launcher
- `CMakeLists.txt` - CMake build configuration

## Requirements
- Windows 10 or later for the supplied launcher
- Python 3
- A C++17-compatible compiler
- A modern web browser

## Run the Project
Open PowerShell in the project directory and run:

```powershell
.\start.bat
```

## Run Tests

```powershell
.\tests\run_tests.ps1
```

## Limitations
This is a simplified educational LEX-inspired implementation, not a complete Lex/Flex replacement or a full C/C++ compiler. It recognizes only the patterns defined in its lexical rules. Advanced numeric formats and complete escape-sequence validation are not supported.

## Academic Purpose
Developed as a Compiler Design mini-project to demonstrate lexical analysis, rule-based scanning, token recognition and lexical error handling.

## Deploy on Render

1. Push this repository to GitHub.
2. Sign in to Render and connect your GitHub account.
3. Create a new Web Service and select `Naina-Ohri/LEX-Tool-CPP`.
4. Select branch `master`.
5. Select the Docker runtime (Render detects the included Dockerfile).
6. Choose the Free instance plan and create the service.
7. Wait for the build and deployment to finish. Render will provide a public `onrender.com` URL.

The Dockerfile installs a C++ compiler, generates the scanner from the project rules, compiles it for the Linux host, and starts the Python web server. The Windows `start.bat` launcher remains available for local use.

Note: Free Render services can spin down after inactivity, so the first visit may take a little longer.

## Live Demo

**Public website:** https://lex-tool-cpp.onrender.com/

**GitHub:** https://github.com/Naina-Ohri/LEX-Tool-CPP

The project is deployed on Render and can be tested directly in a browser.

## Verified Features

- C++17 lexical scanner generator
- Keywords, identifiers and numeric literals
- Character and string literals
- Operators and punctuation
- Comment handling and whitespace skipping
- Lexical errors with line and column reporting
- Browser-based source code editor and token results

## Technologies

- C++17
- Python 3
- HTML, CSS and JavaScript
- Docker
- Render hosting

## Testing

The local scanner pipeline tests passed. Live browser tests verified token recognition, comments and lexical error reporting.

## Run Locally

Run .\start.bat in Windows PowerShell.
