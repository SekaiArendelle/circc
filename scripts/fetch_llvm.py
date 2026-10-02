"""Ensure that the pinned LLVM source tree is available."""

from pathlib import Path
import re
import subprocess
import sys


LLVM_REF = "llvmorg-23.1.2"
LLVM_COMMIT = "85ac560262434c9ccfc0c183ec22d4138ed647fb"
LLVM_REPOSITORY = "https://github.com/llvm/llvm-project.git"
LLVM_VERSION = (23, 1, 2)
CHECKOUT = Path(".deps/llvm-project")


def fail(message: str) -> None:
    print(f"error: {message}", file=sys.stderr)
    raise SystemExit(1)


def check_version() -> None:
    version_file = CHECKOUT / "cmake/Modules/LLVMVersion.cmake"
    try:
        contents = version_file.read_text(encoding="utf-8")
    except OSError as error:
        fail(f"cannot read {version_file}: {error}")

    version = []
    for component in ("MAJOR", "MINOR", "PATCH"):
        match = re.search(
            rf"set\(LLVM_VERSION_{component}\s+(\d+)\)", contents
        )
        if match is None:
            fail(f"cannot determine LLVM version from {version_file}")
        version.append(int(match.group(1)))

    if tuple(version) != LLVM_VERSION:
        actual = ".".join(str(component) for component in version)
        expected = ".".join(str(component) for component in LLVM_VERSION)
        fail(f"{CHECKOUT} contains LLVM {actual}; expected {expected}")


def check_git_revision() -> None:
    result = subprocess.run(
        ["git", "-C", str(CHECKOUT), "rev-parse", "HEAD"],
        check=False,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        fail(f"cannot inspect the Git checkout at {CHECKOUT}")

    actual = result.stdout.strip()
    if actual != LLVM_COMMIT:
        fail(f"{CHECKOUT} is at {actual}; expected {LLVM_REF} ({LLVM_COMMIT})")


def main() -> None:
    if not CHECKOUT.exists():
        CHECKOUT.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(
            [
                "git",
                "clone",
                "--filter=blob:none",
                "--depth",
                "1",
                "--branch",
                LLVM_REF,
                LLVM_REPOSITORY,
                str(CHECKOUT),
            ],
            check=True,
        )
    elif not CHECKOUT.is_dir():
        fail(f"{CHECKOUT} exists but is not a directory")

    check_version()
    if (CHECKOUT / ".git").exists():
        check_git_revision()

    print(f"LLVM {LLVM_REF} is available at {CHECKOUT}")


if __name__ == "__main__":
    main()
