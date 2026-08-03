/**
 * Single source of truth for site-wide SEO metadata.
 *
 * Imported by the root layout, the homepage, robots.ts, sitemap.ts and the
 * Open Graph image so that the canonical URL, titles and descriptions never
 * drift apart across files.
 */
export const siteConfig = {
  name: "SimpleOS",
  // Override this when deploying the optional browser demo.
  url: process.env.NEXT_PUBLIC_SITE_URL ?? "http://127.0.0.1:3160",
  title: "SimpleOS — a Unix-like OS that runs in your browser",
  description:
    "A Unix-like OS built from scratch in C and x86 assembly — boot the real kernel in your browser. Processes, virtual memory, syscalls, a filesystem, and a shell.",
  repoUrl: "https://github.com/raintree-technology/SimpleOS",
  keywords: [
    "SimpleOS",
    "operating system",
    "OS development",
    "osdev",
    "x86 kernel",
    "C and assembly",
    "v86 emulator",
    "browser OS",
    "Unix-like",
    "virtual memory",
    "syscalls",
    "bootloader",
  ],
  author: {
    name: "Zachary Roth",
    url: "https://zacharyr0th.com",
  },
  // Used for JSON-LD `sameAs` and the in-page author links.
  social: {
    x: "https://x.com/zacharyr0th",
    linkedin: "https://www.linkedin.com/in/zacharyr0th",
    github: "https://github.com/zacharyr0th",
  },
  twitterHandle: "@zacharyr0th",
  locale: "en_US",
} as const;
