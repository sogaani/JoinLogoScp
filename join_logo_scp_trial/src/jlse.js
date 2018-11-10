const fs = require("fs");
const path = require("path");
const parseChannel = require("./channel").parse;
const parseParam = require("./param").parse;
const logoframe = require("./command/logoframe").exec;
const chapterexe = require("./command/chapterexe").exec;
const joinlogoframe = require("./command/join_logo_frame").exec;

const usage = () => {
  console.log("node jlse.js input.avs|input.ts");
};

const parseInput = () => {
  if (process.argv.length < 3) {
    usage();
    process.exit(-1);
  }

  const input = process.argv[2];
  const ext = path.extname(input);
  if (ext !== ".ts" && ext !== ".avs") {
    console.error(`invalid file extension ${ext}.`);
    usage();
    process.exit(-1);
  }

  try {
    fs.statSync(input);
  } catch (err) {
    console.error(`File ${input} not found.`);
    usage();
    process.exit(-1);
  }

  return input;
};

const main = () => {
  const filename = parseInput();
  const channel = parseChannel(filename);
  const param = parseParam(channel, filename);

  chapterexe(filename);
  logoframe(param, channel, filename);
  joinlogoframe(param);
};

main();
