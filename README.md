# Hostman

A command line image host manager

## Installation

### Build-time dependencies

- C compiler (GCC or Clang)
- CMake

### Runtime dependencies

- libcurl
- cJSON
- SQLite3
- OpenSSL

### Building from Source

```bash
git clone https://github.com/keircn/hostman && cd hostman
cmake -B build -DHOSTMAN_USE_TUI=ON && cmake --build build
sudo cmake --install build  # optional, installs the binary to /usr/local/bin
```

## Usage

### First Run

When running Hostman for the first time, it will guide you through setting up your first host configuration.

### Basic Commands

```bash
# Upload a file using the default host
hostman upload path/to/file.png

# Upload multiple files at once
hostman upload file1.png file2.jpg file3.gif

# Upload all files from a directory
hostman upload --directory ./screenshots/
hostman upload -d ./images/ --continue-on-error

# List all configured hosts
hostman list-hosts

# Add a new host (interactive)
hostman add-host

# Upload with a specific host
hostman upload --host myhost path/to/file.png

# Remove a host
hostman remove-host myhost

# Set default host
hostman set-default-host myhost

# View upload history
hostman list-uploads

# View upload history with pagination
hostman list-uploads --page 2 --limit 10

# Delete an upload record from local history
hostman delete-upload <id>

# Delete a file from the remote host (if deletion URL is available)
hostman delete-file <id>

# Interactive configuration editor (TUI)
hostman config

# View/modify configuration via CLI
hostman config get log_level
hostman config set log_level DEBUG
```

## Configuration

Hostman uses a JSON configuration file located at `$HOME/.config/hostman/config.json`. The structure is as follows:

```json
{
  "version": 1,
  "default_host": "myhost",
  "log_level": "INFO",
  "log_file": "/path/to/log/file.log",
  "hosts": {
    "myhost": {
      "api_endpoint": "https://example.com/api/upload",
      "auth_type": "header",
      "api_key_name": "Authorization",
      "api_key_encrypted": "...",
      "request_body_format": "multipart",
      "file_form_field": "file",
      "static_form_fields": {
        "folder": "hostman"
      },
      "response_url_json_path": "url",
      "response_deletion_url_json_path": "deletionUrl"
    }
  }
}
```

## File Deletion Support

Hostman supports deletion of files from hosting services that provide deletion URLs in their upload responses. Not all hosts support this feature.

When configuring a host, you can specify the JSON path to the deletion URL in the response using the `response_deletion_url_json_path` field. Leave this blank if the host doesn't support file deletion.

For example, if your hosting service returns:

```json
{
  "success": true,
  "url": "https://example.com/xyz123.png",
  "deletionUrl": "https://example.com/delete?key=abc123"
}
```

You would set:

```
response_url_json_path: url
response_deletion_url_json_path: deletionUrl
```

When you upload a file to a host with deletion URL support:

1. The deletion URL will be displayed and stored in the database
2. In the upload history, records with deletion URLs are marked with `[D]`
3. You can use `hostman delete-file <id>` to delete the file from the remote host

## License

This project is licensed under the MIT License.
