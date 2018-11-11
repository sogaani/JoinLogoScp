const spawnSync = require("child_process").spawnSync;

exports.getFrameRate = filename => {
  const property = exports.getProperty(
    "v:0",
    "stream=avg_frame_rate",
    filename
  );
  const ret = {};
  ret.fpsNumerator = property.match(/^([^/]*)/)[1];
  ret.fpsDenominator = property.match(/\/([^\s]*)/)[1];
  return ret;
};

exports.getSampleRate = filename => {
  const property = exports.getProperty("a:0", "stream=sample_rate", filename);
  const ret = property.match(/([^\s]*)/)[1];
  return ret;
};

exports.getProperty = (stream, entries, filename) => {
  const args = [
    "-v",
    "error",
    "-select_streams",
    stream,
    "-show_entries",
    entries,
    "-of",
    "default=noprint_wrappers=1:nokey=1",
    filename
  ];

  try {
    const result = spawnSync("ffprobe", args);
    return result.stdout.toString();
  } catch (e) {
    console.error(e);
    process.exit(-1);
  }
};
