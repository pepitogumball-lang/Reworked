#!/bin/bash
set -e

# This project uses Node.js only for the preview server (server.js).
# No package.json dependencies to install.
# The C++ mod is built via GitHub Actions, not locally.

echo "Post-merge setup complete."
