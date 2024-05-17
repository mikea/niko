{ pkgs ? import <nixpkgs> {} }:
  pkgs.mkShell {
    nativeBuildInputs = with pkgs.buildPackages; [ 
        # tools
        watchexec just cmake valgrind rlwrap xxd ninja
        # code dependencies
        gcc13 re2c glibc openblas
    ];
    # to compare performance
    packages = [
      (pkgs.python3.withPackages (python-pkgs: [
        python-pkgs.numpy
        python-pkgs.polars
      ]))
    ];
}