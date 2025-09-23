class Uaengine < Formula
  desc "Umicom AuthorEngine AI (uaengine) – CLI to scaffold, build, export and serve book projects"
  homepage "https://github.com/umicom-foundation/umicom-authorengine-ai"
  version "0.1.4"
  license "MIT"

  on_macos do
    url "https://github.com/umicom-foundation/umicom-authorengine-ai/releases/download/v0.1.4/uaengine-macos.zip"
    sha256 "<REPLACE_WITH_SHA256>"
  end

  on_linux do
    url "https://github.com/umicom-foundation/umicom-authorengine-ai/releases/download/v0.1.4/uaengine-linux.zip"
    sha256 "<REPLACE_WITH_SHA256>"
  end

  def install
    if OS.mac?
      bin.install "uaengine"
    else
      bin.install "uaengine"
    end
  end

  test do
    system "#{bin}/uaengine", "--version"
  end
end
