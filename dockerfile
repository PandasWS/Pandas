FROM ubuntu:20.04 AS build
WORKDIR /build
RUN apt update -y
RUN apt install make libmysqlclient-dev musl python3-dev build-essential wget -y
RUN  wget https://github.com/Kitware/CMake/releases/download/v3.16.0-rc1/cmake-3.16.0-rc1.tar.gz \
  && tar -xzvf cmake-3.16.0-rc1.tar.gz \
  && cd cmake-3.16.0-rc1 \
  && ./bootstrap && make -j4 && make install
COPY 3rdparty ./3rdparty
WORKDIR /build/3rdparty/boost
RUN sh ./bootstrap.sh && ./b2
WORKDIR /build
COPY src ./src
COPY CMakeLists.txt ./
COPY LICENSE ./
WORKDIR /build/cbuild
RUN cmake -G "Unix Makefiles" ..
CMD sh
RUN make

FROM ubuntu:20.04 AS login
WORKDIR /app
COPY --from=build /build/login-server ./
RUN apt update -y
RUN apt install libmysqlclient-dev -y
RUN apt-get clean autoclean && apt-get autoremove -y && rm -rf /var/lib/{apt,dpkg,cache,log}/
ENTRYPOINT ["./login-server"]

FROM ubuntu:20.04 AS char
WORKDIR /app
COPY --from=build /build/char-server ./
RUN apt update -y
RUN apt install libmysqlclient-dev -y
RUN apt-get clean autoclean && apt-get autoremove -y && rm -rf /var/lib/{apt,dpkg,cache,log}/
ENTRYPOINT ["./char-server"]

FROM ubuntu:20.04 AS map
WORKDIR /app
COPY --from=build /build/map-server ./
RUN apt update -y
RUN apt install libmysqlclient-dev -y
RUN apt-get clean autoclean && apt-get autoremove -y && rm -rf /var/lib/{apt,dpkg,cache,log}/
ENTRYPOINT ["./map-server"]

FROM ubuntu:20.04 AS web
WORKDIR /app
COPY --from=build /build/web-server ./
RUN apt update -y
RUN apt install libmysqlclient-dev -y
RUN apt-get clean autoclean && apt-get autoremove -y && rm -rf /var/lib/{apt,dpkg,cache,log}/
ENTRYPOINT ["./web-server"]
