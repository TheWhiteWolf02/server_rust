fn main() {
    cc::Build::new()
        .file("src/3rdparty/hmac_sha/sha2.c")
        .file("src/3rdparty/hmac_sha/hmac_sha2.c")
        .file("src/mmc.c")
        .file("src/mmc_cmds.c")
        .file("src/exp.c")
        .compile("mmc");
}