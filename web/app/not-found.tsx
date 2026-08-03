import type { Metadata } from "next";
import Link from "next/link";

/**
 * Custom 404. Vercel already serves this with a real HTTP 404 status (verified),
 * so it is not a soft 404 — and it is explicitly `noindex` so search engines
 * never index the error page itself.
 */
export const metadata: Metadata = {
  title: "Page not found",
  robots: { index: false, follow: false },
};

export default function NotFound() {
  return (
    <main className="flex min-h-screen w-screen flex-col items-center justify-center gap-4 bg-black px-6 text-center text-white">
      <p className="font-mono text-sm uppercase tracking-[0.2em] text-white/50">
        Error 404
      </p>
      <h1 className="text-2xl font-medium">This page could not be found</h1>
      <p className="max-w-prose text-white/60">
        The page you were looking for doesn&apos;t exist. The SimpleOS demo
        lives on the home page.
      </p>
      <Link
        href="/"
        className="mt-2 rounded-full border border-white/20 px-5 py-2 text-sm transition hover:border-white/40 hover:bg-white/5"
      >
        Back to SimpleOS
      </Link>
    </main>
  );
}
