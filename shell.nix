{ pkgs ? import <nixpkgs> {} }:
  pkgs.mkShell {
    nativeBuildInputs = with pkgs.buildPackages; [ 
        watchexec just valgrind
        gcc13 re2c glibc
    ];
    # to compare performance
    packages = [
      (pkgs.python3.withPackages (python-pkgs: [
        python-pkgs.numpy
        python-pkgs.polars
      ]))
    ];
}