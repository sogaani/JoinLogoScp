FROM debian:stretch

ARG CPUCORE="2"
ENV DEV="make gcc git g++ checkinstall automake curl wget autoconf build-essential python2.7 libass-dev libfreetype6-dev libsdl1.2-dev libtheora-dev libtool libva-dev libvdpau-dev libvorbis-dev libxcb1-dev libxcb-shm0-dev libxcb-xfixes0-dev pkg-config texinfo zlib1g-dev"
ENV FFMPEG_VERSION=4.0.2
ENV FFM_VERSION=2.23

RUN apt-get update && \
    apt-get -y install $DEV && \
    apt-get -y install mplayer liblog4cpp5-dev libcairo2-dev libpango1.0-dev && \
    apt-get -y install yasm libx264-dev libmp3lame-dev libopus-dev libva-dev&& \
    apt-get -y install libasound2 libass5 libvdpau1 libva-x11-1 libva-drm1 libxcb-shm0 libxcb-xfixes0 libxcb-shape0 libvorbisenc2 libtheora0 && \
\
#ffmpeg build
\
    mkdir /tmp/ffmpeg_sources && \
    cd /tmp/ffmpeg_sources && \
    wget http://ffmpeg.org/releases/ffmpeg-${FFMPEG_VERSION}.tar.bz2 -O ffmpeg.tar.bz2 && \
    tar xjvf ffmpeg.tar.bz2 && \
    cd /tmp/ffmpeg_sources/ffmpeg* && \
    ./configure \
      --prefix=/usr/local \
      --enable-shared \
      --pkg-config-flags=--static \
      --enable-gpl \
      --enable-libass \
      --enable-libfreetype \
      --enable-libmp3lame \
      --enable-libopus \
      --enable-libtheora \
      --enable-libvorbis \
      --enable-libx264 \
      --enable-nonfree \
      --disable-debug \
      --disable-doc \
    && \
    cd /tmp/ffmpeg_sources/ffmpeg* && \
    make -j${CPUCORE} && \
    make install && \
\
# Install ffms
\
    mkdir /tmp/ffms2_source/ && \
    cd /tmp/ffms2_source/ && \
    git clone https://github.com/sogaani/ffms2_cplugin -b patches --depth=1 && \
    cd /tmp/ffms2_source/ffms2_cplugin && \
    ./configure \
      --enable-shared \
      --enable-avxsynth \
      --enable-pic \
      --prefix=/usr/local \
    && \
    make -j${CPUCORE} && \
    make install && \
\
# Install AvxSynth
\
    mkdir /tmp/AvxSynth/ && \
    cd /tmp/AvxSynth/ && \
    git clone https://github.com/avxsynth/avxsynth.git --depth=1 && \
    cd /tmp/AvxSynth/avxsynth && \
    autoreconf -i && \
    ./configure \
      --disable-ffms2 \
      --enable-shared \
      --prefix=/usr/local \
    && \
    make -j${CPUCORE} && \
    make install

WORKDIR /usr/local/

ENTRYPOINT bash