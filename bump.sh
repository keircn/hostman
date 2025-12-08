#!/bin/bash
set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CMAKE_FILE="$PROJECT_ROOT/CMakeLists.txt"
CHANGELOG_FILE="$PROJECT_ROOT/CHANGELOG.md"
RELEASE_DIR="$PROJECT_ROOT/release"

print_msg() {
  local color=$1
  local msg=$2
  echo -e "${color}${msg}${NC}"
}

get_current_version() {
  grep -Po '(?<=project\(hostman VERSION )[0-9]+\.[0-9]+\.[0-9]+' "$CMAKE_FILE"
}

bump_version() {
  local version=$1
  local bump_type=$2

  local major minor patch
  IFS='.' read -r major minor patch <<<"$version"

  case $bump_type in
  major)
    major=$((major + 1))
    minor=0
    patch=0
    ;;
  minor)
    minor=$((minor + 1))
    patch=0
    ;;
  patch)
    patch=$((patch + 1))
    ;;
  *)
    print_msg "$RED" "Invalid bump type: $bump_type (use major, minor, or patch)"
    exit 1
    ;;
  esac

  echo "${major}.${minor}.${patch}"
}

update_cmake_version() {
  local new_version=$1
  sed -i "s/project(hostman VERSION [0-9]\+\.[0-9]\+\.[0-9]\+/project(hostman VERSION $new_version/" "$CMAKE_FILE"
  print_msg "$GREEN" "Updated CMakeLists.txt to version $new_version"
}

update_changelog() {
  local new_version=$1
  local date_str
  date_str=$(date +%Y-%m-%d)
  local temp_file
  temp_file=$(mktemp)
  head -n 6 "$CHANGELOG_FILE" >"$temp_file"
  cat >>"$temp_file" <<EOF

## [$new_version] - $date_str

### Added

- 

### Fixed

- 
EOF
  tail -n +7 "$CHANGELOG_FILE" >>"$temp_file"
  mv "$temp_file" "$CHANGELOG_FILE"
  print_msg "$GREEN" "Added changelog entry for version $new_version"
}

get_native_platform() {
  local os arch
  os=$(uname -s)
  arch=$(uname -m)

  case "$os" in
  Linux)
    case "$arch" in
    x86_64) echo "linux-x86_64" ;;
    aarch64) echo "linux-arm64" ;;
    *) echo "linux-$arch" ;;
    esac
    ;;
  Darwin)
    echo "macos"
    ;;
  MINGW* | MSYS* | CYGWIN*)
    echo "windows"
    ;;
  *)
    echo "unknown"
    ;;
  esac
}

build_native() {
  local version=$1
  local platform
  platform=$(get_native_platform)
  local build_dir="$RELEASE_DIR/$platform"

  print_msg "$BLUE" "Building for $platform (native)..."

  mkdir -p "$build_dir"
  cd "$build_dir"

  local make_jobs
  if [[ "$(uname)" == "Darwin" ]]; then
    make_jobs=$(sysctl -n hw.ncpu)
  else
    make_jobs=$(nproc)
  fi

  if cmake "$PROJECT_ROOT" -DCMAKE_BUILD_TYPE=Release && make -j"$make_jobs"; then
    cd "$PROJECT_ROOT"
    print_msg "$GREEN" "Successfully built for $platform"
    echo "$platform"
    return 0
  else
    cd "$PROJECT_ROOT"
    print_msg "$RED" "Build failed for $platform"
    return 1
  fi
}

create_archive() {
  local platform=$1
  local version=$2
  local build_dir="$RELEASE_DIR/$platform"
  local archive_name="hostman-$version-$platform"

  if [[ ! -d "$build_dir" ]]; then
    print_msg "$YELLOW" "Build directory for $platform not found, skipping archive"
    return 1
  fi

  print_msg "$BLUE" "Creating archive for $platform..."

  cp "$PROJECT_ROOT/LICENSE" "$build_dir/"
  cp "$PROJECT_ROOT/README.md" "$build_dir/"
  cp "$PROJECT_ROOT/CHANGELOG.md" "$build_dir/"

  cd "$RELEASE_DIR"

  if [[ "$platform" == "windows" ]]; then
    local binary_name="hostman.exe"
    if [[ -f "$build_dir/$binary_name" ]]; then
      if command -v zip &>/dev/null; then
        zip -j "$archive_name.zip" \
          "$build_dir/$binary_name" \
          "$build_dir/LICENSE" \
          "$build_dir/README.md" \
          "$build_dir/CHANGELOG.md"
        print_msg "$GREEN" "Created $archive_name.zip"
      elif command -v 7z &>/dev/null; then
        7z a "$archive_name.zip" \
          "$build_dir/$binary_name" \
          "$build_dir/LICENSE" \
          "$build_dir/README.md" \
          "$build_dir/CHANGELOG.md"
        print_msg "$GREEN" "Created $archive_name.zip"
      else
        print_msg "$YELLOW" "Neither zip nor 7z found, skipping Windows archive"
        cd "$PROJECT_ROOT"
        return 1
      fi
    else
      print_msg "$YELLOW" "Windows binary not found, skipping archive"
      cd "$PROJECT_ROOT"
      return 1
    fi
  else
    local binary_name="hostman"
    if [[ -f "$build_dir/$binary_name" ]]; then
      tar -czvf "$archive_name.tar.gz" \
        -C "$build_dir" \
        "$binary_name" \
        "LICENSE" \
        "README.md" \
        "CHANGELOG.md"
      print_msg "$GREEN" "Created $archive_name.tar.gz"
    else
      print_msg "$YELLOW" "Binary for $platform not found, skipping archive"
      cd "$PROJECT_ROOT"
      return 1
    fi
  fi

  cd "$PROJECT_ROOT"
  return 0
}

usage() {
  echo "Usage: $0 [OPTIONS] <major|minor|patch>"
  echo ""
  echo "Options:"
  echo "  -h, --help      Show this help message"
  echo "  -s, --skip-build Skip building binaries (only bump version)"
  echo "  -c, --clean     Clean release directory before building"
  echo ""
  echo "Examples:"
  echo "  $0 patch        # Bump patch version (1.1.5 -> 1.1.6)"
  echo "  $0 minor        # Bump minor version (1.1.5 -> 1.2.0)"
  echo "  $0 major        # Bump major version (1.1.5 -> 2.0.0)"
  echo "  $0 -s patch     # Only bump version, don't build"
}

main() {
  local skip_build=false
  local clean=false
  local bump_type=""

  while [[ $# -gt 0 ]]; do
    case $1 in
    -h | --help)
      usage
      exit 0
      ;;
    -s | --skip-build)
      skip_build=true
      shift
      ;;
    -c | --clean)
      clean=true
      shift
      ;;
    major | minor | patch)
      bump_type=$1
      shift
      ;;
    *)
      print_msg "$RED" "Unknown option: $1"
      usage
      exit 1
      ;;
    esac
  done

  if [[ -z "$bump_type" ]]; then
    print_msg "$RED" "Error: bump type required (major, minor, or patch)"
    usage
    exit 1
  fi

  local current_version
  current_version=$(get_current_version)
  print_msg "$BLUE" "Current version: $current_version"

  local new_version
  new_version=$(bump_version "$current_version" "$bump_type")
  print_msg "$GREEN" "New version: $new_version"

  update_cmake_version "$new_version"
  update_changelog "$new_version"

  if [[ "$skip_build" == true ]]; then
    print_msg "$YELLOW" "Skipping build (--skip-build specified)"
    print_msg "$GREEN" "Version bumped to $new_version"
    print_msg "$BLUE" "Don't forget to:"
    echo "  1. Edit CHANGELOG.md to add your changes"
    echo "  2. Commit: git add CMakeLists.txt CHANGELOG.md && git commit -m 'Bump version to $new_version'"
    echo "  3. Tag: git tag -a v$new_version -m 'Version $new_version'"
    echo "  4. Push: git push origin main --tags"
    exit 0
  fi

  if [[ "$clean" == true ]]; then
    print_msg "$YELLOW" "Cleaning release directory..."
    rm -rf "$RELEASE_DIR"
  fi

  mkdir -p "$RELEASE_DIR"

  local built_platform
  if built_platform=$(build_native "$new_version"); then
    print_msg "$BLUE" "Creating release archive..."
    create_archive "$built_platform" "$new_version"
  else
    print_msg "$RED" "Build failed"
    exit 1
  fi

  echo ""
  print_msg "$GREEN" "=== Release $new_version Summary ==="
  print_msg "$BLUE" "Version bumped: $current_version -> $new_version"
  print_msg "$GREEN" "Built for: $built_platform"

  echo ""
  print_msg "$BLUE" "Release archive created in: $RELEASE_DIR/"
  ls -la "$RELEASE_DIR"/*.{tar.gz,zip} 2>/dev/null || true

  echo ""
  print_msg "$BLUE" "Next steps:"
  echo "  1. Edit CHANGELOG.md to add your changes"
  echo "  2. Commit: git add CMakeLists.txt CHANGELOG.md && git commit -m 'Bump version to $new_version'"
  echo "  3. Tag: git tag -a v$new_version -m 'Version $new_version'"
  echo "  4. Push: git push origin main --tags"
  echo "  5. Upload archives from $RELEASE_DIR/ to GitHub Releases"
}

main "$@"
