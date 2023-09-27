FROM rust:latest

RUN mkdir /server_rust

ADD ./src /server_rust/src/
ADD ./Cargo.toml /server_rust/
ADD ./Cargo.lock /server_rust/
ADD ./build.sh /server_rust/

WORKDIR /server_rust

EXPOSE 24000

CMD sh build.sh