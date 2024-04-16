set -e

cd $(pwd)/build 
make 
./linear_algebra_tests
