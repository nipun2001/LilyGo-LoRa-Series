Import("env")

from os.path import isabs, isdir, join

custom_src_dir = env.GetProjectOption("custom_src_dir", "").strip()
if not custom_src_dir:
    Return()

src_dir = custom_src_dir if isabs(custom_src_dir) else join(env["PROJECT_DIR"], custom_src_dir)
if not isdir(src_dir):
    env.Exit(f"custom_src_dir does not exist: {src_dir}")

env.Replace(PROJECT_SRC_DIR=src_dir)
print(f"Using source directory: {src_dir}")
