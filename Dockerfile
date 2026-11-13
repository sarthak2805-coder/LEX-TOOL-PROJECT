FROM python:3.12-slim
RUN apt-get update && apt-get install -y --no-install-recommends g++ && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY . .
RUN mkdir -p build && g++ -std=c++17 -O2 -Wall -Wextra -pedantic src/lexgen_v2.cpp -o build/lexgen_v2 && ./build/lexgen_v2 tests/lex_style_sample.lex src/lex_style_scanner.cpp && g++ -std=c++17 -O2 -Wall -Wextra -pedantic src/lex_style_scanner.cpp -o build/lex_style_scanner
ENV PYTHONUNBUFFERED=1
CMD ["python", "server.py"]
