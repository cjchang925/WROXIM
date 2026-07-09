export LD_LIBRARY_PATH="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/../systemc-3.0.2/lib-linux64:${LD_LIBRARY_PATH}"
#gdb ./noxim
#./noxim -config ../tm_examples/01-16-nownoc.yaml -taskmapping ../tm_examples_hybrid/01-oneapp-seventasks1.map -taskmappinglog ../tm_log_hybrid/tmp.log > ../tm_log_hybrid/tmp_output.log
./noxim -config ../tm_examples/01-16-nownoc.yaml -taskmapping ../tm_examples_hybrid/test-app.map -taskmappinglog ../tm_log_hybrid/tmp.log > ../tm_log_hybrid/tmp_output.log
#run -config ../tm_examples/01-16-nownoc.yaml -taskmapping ../tm_examples_hybrid/01-oneapp-seventasks1.map -taskmappinglog ../tm_log_hybrid/tmp.log > ../tm_log_hybrid/tmp_output.log