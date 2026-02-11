# Kafka CLI Tool

![Build and Release](https://github.com/zateckar/kafka-test/workflows/build.yml/badge.svg)

A command-line application for testing Kafka brokers with mutual TLS (mTLS) authentication. Built in C using the librdkafka library.

## Features

- **Producer Mode**: Send test messages to a Kafka topic
- **Consumer Mode**: Receive and display messages from a Kafka topic
- **mTLS Authentication**: Secure communication with client certificates
- **Interactive TUI**: Menu-driven interface for easy operation
- **Verbose Logging**: Detailed logging for debugging broker connections
- **INI Configuration**: All settings managed through configuration file

## Prerequisites

### For Running Pre-built Binaries
- Windows 10/11 (x64)
- No additional dependencies required (all DLLs included)

### For Building from Source
- **MinGW-w64** or **MSYS2** with GCC compiler
- **librdkafka** DLLs (included in `librdkafka/` directory)


## Configuration

Copy `kafka_config.ini` and customize for your environment:

```ini
[broker]
brokers = your-kafka-broker:9093
topic = your-topic-name

[mTLS]
ssl_ca_location = certs/ca-cert.pem
ssl_certificate_location = certs/client-cert.pem
ssl_key_location = certs/client-key.pem
ssl_key_password = your-key-password  ; Optional
```

### Configuration Sections

| Section | Description |
|---------|-------------|
| `[broker]` | Kafka broker address and topic |
| `[mTLS]` | SSL/TLS certificate paths and settings |
| `[producer]` | Producer-specific settings (batch size, acks, etc.) |
| `[consumer]` | Consumer-specific settings (group ID, offset reset) |
| `[general]` | General settings (verbose, message count) |

## Usage

### Interactive Mode (TUI)

Run without arguments to launch the interactive menu:

```cmd
kafka_cli.exe
```

### Command Line Mode

#### Show Help
```cmd
kafka_cli.exe -h
```

#### Show Version
```cmd
kafka_cli.exe -V
```

#### Run as Producer
```cmd
kafka_cli.exe produce
kafka_cli.exe -v -m 100 produce
kafka_cli.exe -c custom_config.ini produce
```

#### Run as Consumer
```cmd
kafka_cli.exe consume
kafka_cli.exe -v consume
kafka_cli.exe -m 50 consume
```

### Command Line Options

| Option | Description |
|--------|-------------|
| `-c <file>` | Specify configuration file (default: `kafka_cli.ini`) |
| `-m <num>` | Number of messages to produce/consume |
| `-v` | Enable verbose logging |
| `-V` | Show version information |
| `-h` | Show help |

## Logging

The application creates timestamped log files in the `logs/` directory:
- Filename format: `topic-name_mode_YYYYMMDD_HHMMSS.log`
- Console output with color-coded log levels
- Verbose mode (`-v`) enables detailed debug information

### Log Levels

- `[INFO]` - General information
- `[CONFIG]` - Configuration settings
- `[DEBUG]` - Detailed debug information (only with `-v` flag)
- `[ERROR]` - Error messages

Example output:
```
[2026-02-11 17:30:45] [INFO] Built with librdkafka 2.3.0
[2026-02-11 17:30:45] [CONFIG] Brokers: localhost:9093
[2026-02-11 17:30:45] [INFO] Produced message 1/10: Test message 1...
```

## Setting Up mTLS

### Generating Test Certificates

1. **Create CA certificate:**
```bash
openssl req -x509 -newkey rsa:4096 -keyout ca-key.pem -out ca-cert.pem -days 365 -nodes -subj "/CN=Kafka-Test-CA"
```

2. **Create client key and certificate:**
```bash
openssl req -newkey rsa:4096 -keyout client-key.pem -out client-req.pem -nodes -subj "/CN=kafka-client"
openssl x509 -req -in client-req.pem -CA ca-cert.pem -CAkey ca-key.pem -out client-cert.pem -days 365 -CAcreateserial
```

3. **Place certificates in `certs/` directory and update `kafka_config.ini`**

## License

This is a test tool for Kafka broker validation.

## Acknowledgments

- Built with [librdkafka](https://github.com/edenhill/librdkafka) - the Apache Kafka C/C++ client library
- librdkafka is licensed under the 2-clause BSD license.
