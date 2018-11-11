const fs = require("fs");
const ffprobe = require("../command/ffprobe");

exports.create = (tsFile, trimFile, outputFile) => {
  try {
    const trimString = fs.readFileSync(trimFile).toString();
    let result;
    const reg = /Trim\((\d*)\,(\d*)/g;
    const trimFrames = [];

    while ((result = reg.exec(trimString))) {
      const trimFrame = {
        start: result[1],
        end: result[2]
      };
      trimFrames.push(trimFrame);
    }

    const sampleRate = ffprobe.getSampleRate(tsFile);
    const fps = ffprobe.getFrameRate(tsFile);

    let filterString = "";
    let concatString = "";
    for (let i = 0; i < trimFrames.length; i++) {
      const trimFrame = trimFrames[i];
      filterString += `[0:v]trim=start_frame=${trimFrame.start}:end_frame=${
        trimFrame.end
      },setpts=PTS-STARTPTS[v${i}];`;

      const startSample = parseInt(
        (trimFrame.start * sampleRate * fps.fpsDenominator) / fps.fpsNumerator +
          0.5,
        10
      );
      const endSample = parseInt(
        (trimFrame.end * sampleRate * fps.fpsDenominator) / fps.fpsNumerator +
          0.5,
        10
      );
      filterString += `[0:a]atrim=start_sample=${startSample}:end_sample=${endSample},asetpts=PTS-STARTPTS[a${i}];`;
      concatString += `[v${i}][a${i}]`;
    }
    filterString += `${concatString}concat=n=${
      trimFrames.length
    }:v=1:a=1[video][audio];`;

    fs.writeFileSync(outputFile, filterString);
  } catch (e) {
    console.error(e);
    process.exit(-1);
  }
};
