// Copyright 2016 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.!

#include <functional>
#include "common.h"
#include "flags.h"
#include "sentencepiece.pb.h"
#include "sentencepiece_processor.h"
#include "util.h"

DEFINE_string(model, "", "model file name");
DEFINE_string(
    output_format, "piece",
    "choose from piece, id, proto, nbest_piece, nbest_id, or nbest_proto");
DEFINE_string(output, "", "output filename");
DEFINE_string(extra_options, "",
              "':' separated encoder extra options, e.g., \"reverse:bos:eos\"");
DEFINE_int32(nbest_size, 10, "NBest size");
DEFINE_double(alpha, 0.5, "Smoothing parameter for sampling mode.");

int main(int argc, char *argv[]) {
  std::vector<std::string> rest_args;
  sentencepiece::flags::ParseCommandLineFlags(argc, argv, &rest_args);

  CHECK_OR_HELP(model);

  sentencepiece::SentencePieceProcessor sp;
  CHECK_OK(sp.Load(FLAGS_model));
  CHECK_OK(sp.SetEncodeExtraOptions(FLAGS_extra_options));

  sentencepiece::io::OutputBuffer output(FLAGS_output);

  if (rest_args.empty()) {
    rest_args.push_back("");  // empty means that reading from stdin.
  }

  std::string line;
  std::vector<std::string> sps;
  std::vector<int> ids;
  std::vector<std::vector<std::string>> nbest_sps;
  std::vector<std::vector<int>> nbest_ids;
  sentencepiece::SentencePieceText spt;
  sentencepiece::NBestSentencePieceText nbest_spt;
  std::function<void(const std::string &line)> process;

  if (FLAGS_output_format == "piece") {
    process = [&](const std::string &line) {
      CHECK_OK(sp.Encode(line, &sps));
      output.WriteLine(sentencepiece::string_util::Join(sps, " "));
    };
  } else if (FLAGS_output_format == "id") {
    process = [&](const std::string &line) {
      CHECK_OK(sp.Encode(line, &ids));
      output.WriteLine(sentencepiece::string_util::Join(ids, " "));
    };
  } else if (FLAGS_output_format == "proto") {
    process = [&](const std::string &line) {
      CHECK_OK(sp.Encode(line, &spt));
      output.WriteLine(spt.Utf8DebugString());
    };
  } else if (FLAGS_output_format == "sample_piece") {
    process = [&](const std::string &line) {
      CHECK_OK(sp.SampleEncode(line, FLAGS_nbest_size, FLAGS_alpha, &sps));
      output.WriteLine(sentencepiece::string_util::Join(sps, " "));
    };
  } else if (FLAGS_output_format == "sample_id") {
    process = [&](const std::string &line) {
      CHECK_OK(sp.SampleEncode(line, FLAGS_nbest_size, FLAGS_alpha, &ids));
      output.WriteLine(sentencepiece::string_util::Join(ids, " "));
    };
  } else if (FLAGS_output_format == "sample_proto") {
    process = [&](const std::string &line) {
      CHECK_OK(sp.SampleEncode(line, FLAGS_nbest_size, FLAGS_alpha, &spt));
      output.WriteLine(spt.Utf8DebugString());
    };
  } else if (FLAGS_output_format == "nbest_piece") {
    process = [&](const std::string &line) {
      CHECK_OK(sp.NBestEncode(line, FLAGS_nbest_size, &nbest_sps));
      for (const auto &result : nbest_sps) {
        output.WriteLine(sentencepiece::string_util::Join(result, " "));
      }
    };
  } else if (FLAGS_output_format == "nbest_id") {
    process = [&](const std::string &line) {
      CHECK_OK(sp.NBestEncode(line, FLAGS_nbest_size, &nbest_ids));
      for (const auto &result : nbest_ids) {
        output.WriteLine(sentencepiece::string_util::Join(result, " "));
      }
    };
  } else if (FLAGS_output_format == "nbest_proto") {
    process = [&](const std::string &line) {
      CHECK_OK(sp.NBestEncode(line, FLAGS_nbest_size, &nbest_spt));
      output.WriteLine(nbest_spt.Utf8DebugString());
    };
  } else {
    LOG(FATAL) << "Unknown output format: " << FLAGS_output_format;
  }

  for (const auto &filename : rest_args) {
    sentencepiece::io::InputBuffer input(filename);
    while (input.ReadLine(&line)) {
      if (line.empty()) {
        output.WriteLine("");
        continue;
      }
      process(line);
    }
  }

  return 0;
}
