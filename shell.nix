{ pkgs ? import <nixpkgs> {} }:
  pkgs.mkShell {
    nativeBuildInputs = with pkgs.buildPackages; [ 
        watchexec just valgrind
        gcc13 re2c glibc
    ];
}