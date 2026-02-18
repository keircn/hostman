{
  description = "hostman: image host manager";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
      in
      {
        packages = rec {
          hostman = pkgs.stdenv.mkDerivation {
            pname = "hostman";
            version = "1.2.2";

            src = self;

            nativeBuildInputs = [
              pkgs.cmake
              pkgs.pkg-config
            ];

            buildInputs = [
              pkgs.curl
              pkgs.sqlite
              pkgs.cjson
              pkgs.openssl
              pkgs.ncurses
            ];

            cmakeFlags = [
              "-DHOSTMAN_USE_TUI=ON"
            ];

            meta = with pkgs.lib; {
              description = "A command-line image host manager";
              homepage = "https://github.com/keircn/hostman";
              license = licenses.mit;
              platforms = platforms.unix;
              mainProgram = "hostman";
            };
          };

          default = hostman;
        };

        apps = rec {
          hostman = flake-utils.lib.mkApp {
            drv = self.packages.${system}.hostman;
          };
          default = hostman;
        };

        devShells.default = pkgs.mkShell {
          inputsFrom = [ self.packages.${system}.hostman ];

          packages = [
            pkgs.clang-tools
          ];
        };
      });
}
