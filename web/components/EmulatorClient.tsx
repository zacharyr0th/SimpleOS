"use client";

import dynamic from "next/dynamic";

/**
 * Client-only mount point for the emulator.
 *
 * The emulator touches `window`, loads the v86 WebAssembly core and can only
 * run in the browser, so it is imported with `ssr: false`. Keeping that import
 * isolated here lets `app/page.tsx` stay a Server Component, which is what puts
 * the crawlable heading, description and JSON-LD into the static HTML.
 */
const Emulator = dynamic(() => import("./Emulator"), {
  ssr: false,
  loading: () => (
    <div className="w-screen h-screen bg-black flex flex-col items-center justify-center text-white">
      <div className="text-xl mb-2">Spinning up the runtime…</div>
    </div>
  ),
});

export default function EmulatorClient() {
  return <Emulator />;
}
