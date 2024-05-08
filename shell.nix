{ pkgs ? import <nixpkgs> {} }:
  pkgs.mkShell {
    nativeBuildInputs = with pkgs.buildPackages; [ 
        # code dependencies
        gcc13 re2c glibc
        # build dependencies
        watchexec just cmake
        # tools
        valgrind
    ];
    # to compare performance
    packages = [
      (pkgs.python3.withPackages (python-pkgs: [
        python-pkgs.numpy
        python-pkgs.polars
      ]))
    ];
}