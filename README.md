# Hostman - Image host manager

Hostman is a robust, cross-platform command-line application for uploading files (primarily images) to various hosting services.

## Features

- Upload files to multiple configurable hosting services
- Manage hosting service configurations
- Track upload history using SQLite
- Support for file deletion via host-provided deletion URLs
- Visual progress bar during uploads

## Installation

### Build-time dependencies

- C compiler (GCC or Clang)
- CMake
- libcurl
- cJSON
- SQLite3

### Building from Source

```bash
git clone https://github.com/Bestire/hostman && cd hostman
cmake --build build

# Install (optional)
cd build && sudo make install
```

## Usage

### First Run

When running Hostman for the first time, it will guide you through setting up your first host configuration.

Basic Commands

```bash
# Upload a file using the default host
hostman upload path/to/file.png

# List all configured hosts
hostman list-hosts

# Add a new host (interactive)
hostman add-host

# Upload with a specific host
hostman upload --host nest.rip path/to/file.png

# Remove a host
hostman remove-host nest.rip

# Set default host
hostman set-default-host nest.rip

# View upload history
hostman list-uploads

# View upload history with pagination
hostman list-uploads --page 2 --limit 10

# Delete an upload record from local history
hostman delete-upload <id>

# Delete a file from the remote host (if deletion URL is available)
hostman delete-file <id>

# View/modify configuration
hostman config get log_level
hostman config set log_level DEBUG
```

## Configuration

Hostman uses a JSON configuration file located at `$HOME/.config/hostman/config.json`. The structure is as follows:

```json
{
  "version": 1,
  "default_host": "nest.rip",
  "log_level": "INFO",
  "log_file": "/path/to/log/file.log",
  "hosts": {
    "anonhost_personal": {
      "api_endpoint": "https://nest.rip/api/files",
      "auth_type": "bearer",
      "api_key_name": "Authorization",
      "api_key_encrypted": "...",
      "request_body_format": "multipart",
      "file_form_field": "file",
      "static_form_fields": {
        "folder": "hostman"
      },
      "response_url_json_path": "fileURL",
      "response_deletion_url_json_path": "deletionURL"
    }
  }
}
```

## File Deletion Support

Hostman now supports deletion of files from hosting services that provide deletion URLs in their upload responses. When configuring a host, you can specify the JSON path to the deletion URL in the response using the `response_deletion_url_json_path` field.

For example, if your hosting service returns:

```json
{
  "success": true,
  "message": "File Uploaded",
  "fileUrl": "https://example.com/xyz123.png",
  "deletionUrl": "https://example.com/delete?key=random-deletion-key"
}
```

You would set:

```
response_url_json_path: "fileUrl"
response_deletion_url_json_path: "deletionUrl"
```

When you upload a file to a host with deletion URL support:

1. The deletion URL will be displayed and stored in the database
2. In the upload history, records with deletion URLs are marked with [ID: X]
3. You can use `hostman delete-file <id>` to delete the file from the remote host

## License

This project is licensed under the MIT License.
