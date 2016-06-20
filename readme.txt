1.源码编译方式
  1. cd app/
  2. ./gen_misc.sh 
  
2.库编译方式
  1. 先用源码编译方式，生成libgagent.a 库路径为:"app/gagent/.output/eagle/debug/lib"
  2. cp app/gagent/.output/eagle/debug/lib/libgagent.a ../lib/
  3. mv makefile makefile_src
  4. mv makefile_lib makefile
  5. ./gen_misc.sh 