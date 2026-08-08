this implementation follows llvm's [this](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/index.html) tutorial.


build:

```
cmake -S . -B build \
  -DLLVM_DIR=$HOME/Developer/llvm-project/build/lib/cmake/llvm

cmake --build build
```
