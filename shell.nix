{ pkgs ? import <nixpkgs> {} }:
  pkgs.mkShell {
    nativeBuildInputs = with pkgs.buildPackages; [ 
        # tools
        watchexec just cmake valgrind rlwrap xxd ninja
        # build time code dependencies
        gcc13 re2c openblas jemalloc libgcc lcov
    ];
    # to compare performance
    packages = [
      (pkgs.python3.withPackages (python-pkgs: [
        python-pkgs.numpy
        python-pkgs.polars
      ]))
    ];
}