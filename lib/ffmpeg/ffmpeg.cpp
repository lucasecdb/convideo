#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <stdio.h>

using namespace emscripten;

extern "C" int main(int argc, char** argv);

uint8_t* result;

val convert(std::string video) {
  remove("input.mp4");
  remove("output.mkv");

  FILE* infile = fopen("input.mp4", "wb");
  fwrite(video.c_str(), video.length(), 1, infile);
  fflush(infile);
  fclose(infile);

  char* args[] = { "ffmpeg", "-i", "input.mp4", "output.mkv" };
  main(4, args);

  FILE *outfile = fopen("output.mkv", "rb");
  fseek(outfile, 0, SEEK_END);
  int fsize = ftell(outfile);
  result = (uint8_t*) malloc(fsize);
  fseek(outfile, 0, SEEK_SET);
  fread(result, fsize, 1, outfile);
  return val(typed_memory_view(fsize, result));
}

void free_result() {
  free(result);
}

EMSCRIPTEN_BINDINGS(ffmpeg) {
  function("free_result", &free_result);
  function("convert", &convert);
}
