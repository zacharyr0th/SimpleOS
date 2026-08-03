import path from "node:path";
import type { NextConfig } from "next";

const workspaceRoot = path.resolve(import.meta.dirname, "../../..");

const nextConfig: NextConfig = {
  poweredByHeader: false,
  experimental: {
    turbopackFileSystemCacheForBuild: true,
  },
  turbopack: {
    root: workspaceRoot,
  },
  output: "export",
  // Canonical URLs have no trailing slash (matches vercel.json + rel=canonical).
  trailingSlash: false,
};

export default nextConfig;
