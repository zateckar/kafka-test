# Kafka CLI Tool

[![Build and Release](https://github.com/USERNAME/REPO_NAME/actions/workflows/build.yml/badge.svg)](https://github.com/USERNAME/REPO_NAME/actions/workflows/build.yml)

A command-line application for testing Kafka brokers with mutual TLS (mTLS) authentication. Built in C using the librdkafka library.

## Features

- **Producer Mode**: Send test messages to a Kafka topic
- **Consumer Mode**: Receive and display messages from a Kafka topic
- **mTLS Authentication**: Secure communication with client certificates
- **Interactive TUI**: Menu-driven interface for easy operation
- **Verbose Logging**: Detailed logging for debugging broker connections
- **INI Configuration**: All settings managed through configuration file
- **Cross-Platform**: Supports Windows (built with MinGW/GCC)

## Project Structure

```
kafka-test/
├── src/
│   └── kafka_cli.c          # Main source code
├── librdkafka/              # Pre-built librdkafka DLLs
│   ├── librdkafka.dll
│   ├── libssl-3-x64.dll
│   ├── libcrypto-3-x64.dll
│   └── ...
├── certs/                   # Client certificates (not in repo)
├── build.bat                # Build script
├── kafka_config.ini         # Sample configuration file
├── .github/
│   └── workflows/
│       └── build.yml        # CI/CD pipeline
└── README.md                # This file
```

## Download

Pre-built binaries are available on the [Releases](../../releases) page.

Download the latest `KafkaCLI.zip` and extract to your desired location.

## Prerequisites

### For Running Pre-built Binaries
- Windows 10/11 (x64)
- No additional dependencies required (all DLLs included)

### For Building from Source
- **MinGW-w64** or **MSYS2** with GCC compiler
- **librdkafka** DLLs (included in `librdkafka/` directory)

### Installing MinGW-w64 on Windows

1. Download from: https://www.mingw-w64.org/downloads/
2. Or use MSYS2: `pacman -S mingw-w64-x86_64-gcc`
3. Add GCC to your PATH

## Building from Source

### Local Build

Run the build script:

```cmd
build.bat
```

This will:
1. Compile the source code with GCC
2. Link against librdkafka dynamically
3. Copy required DLLs to the `build/` directory
4. Create `build/kafka_cli.exe`

### CI/CD Build

The project uses GitHub Actions for automated builds:

- **Automatic builds** on push to `main`/`master` branches
- **Tagged releases** when pushing tags like `v1.0.0`
- **Artifacts** available for 30 days on every build

See [`.github/workflows/build.yml`](.github/workflows/build.yml) for details.

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

### Kafka Broker SSL Configuration

Ensure your `server.properties` includes:

```properties
listeners=SSL://:9093
security.inter.broker.protocol=SSL
ssl.keystore.location=/path/to/server.keystore.jks
ssl.keystore.password=your-password
ssl.key.password=your-password
ssl.truststore.location=/path/to/server.truststore.jks
ssl.truststore.password=your-password
ssl.client.auth=required
```

## Troubleshooting

### "Failed to create producer/consumer"
- Check that all certificate paths are correct
- Verify the CA certificate is valid
- Ensure the broker is running and accessible

### SSL handshake errors
- Verify certificates are in PEM format
- Check that the client certificate was signed by the CA
- Ensure `ssl_key_password` is correct if key is encrypted

### Missing DLL errors
- Run from the same directory where DLLs are located
- Or ensure all DLLs from the release package are present

## Contributing

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/my-feature`
3. Commit your changes: `git commit -am 'Add new feature'`
4. Push to the branch: `git push origin feature/my-feature`
5. Submit a pull request

## Release Process

To create a new release:

1. Update version in `src/kafka_cli.c` (`#define VERSION`)
2. Commit changes: `git commit -am "Bump version to X.Y.Z"`
3. Create and push a tag: `git tag -a vX.Y.Z -m "Release X.Y.Z"`
4. Push the tag: `git push origin vX.Y.Z`
5. GitHub Actions will automatically build and create a release

## License

This is a test tool for Kafka broker validation.

## Acknowledgments

- Built with [librdkafka](https://github.com/edenhill/librdkafka) - the Apache Kafka C/C++ client library
