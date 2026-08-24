this implementation follows llvm's [this](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/index.html) tutorial.

additional features which do not exists in tutorial:
- mutable global variables
- print function to print variables

build:

```
cmake -S . -B build \
  -DLLVM_DIR=<path_to_project>/llvm-project/build/lib/cmake/llvm

cmake --build build
```
