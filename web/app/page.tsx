import EmulatorClient from "@/components/EmulatorClient";
import { siteConfig } from "@/lib/site";

/**
 * JSON-LD (schema.org) describing the page as a free, open-source
 * SoftwareApplication authored by a Person, plus the WebSite itself. Rendered
 * server-side so it is present in the static HTML for search engines and AI
 * agents.
 */
const jsonLd = {
  "@context": "https://schema.org",
  "@graph": [
    {
      "@type": "WebSite",
      "@id": `${siteConfig.url}/#website`,
      url: `${siteConfig.url}/`,
      name: siteConfig.name,
      description: siteConfig.description,
      inLanguage: "en",
      publisher: { "@id": `${siteConfig.url}/#author` },
    },
    {
      "@type": "SoftwareApplication",
      "@id": `${siteConfig.url}/#software`,
      name: siteConfig.name,
      url: `${siteConfig.url}/`,
      description: siteConfig.description,
      applicationCategory: "DeveloperApplication",
      operatingSystem: "Any (runs in a web browser)",
      browserRequirements: "Requires JavaScript and WebAssembly",
      softwareVersion: "0.1.0",
      isAccessibleForFree: true,
      offers: { "@type": "Offer", price: "0", priceCurrency: "USD" },
      license: "https://opensource.org/licenses/MIT",
      codeRepository: siteConfig.repoUrl,
      image: `${siteConfig.url}/og.png`,
      author: { "@id": `${siteConfig.url}/#author` },
    },
    {
      "@type": "Person",
      "@id": `${siteConfig.url}/#author`,
      name: siteConfig.author.name,
      url: siteConfig.author.url,
      sameAs: [
        siteConfig.social.x,
        siteConfig.social.linkedin,
        siteConfig.social.github,
      ],
    },
  ],
};

export default function Home() {
  return (
    <>
      {/* JSON-LD structured data. Rendered as escaped text children rather than
          dangerouslySetInnerHTML: the object is static and trusted and contains
          no characters that HTML-escaping would alter, so the output is
          identical without needing a security-lint suppression. */}
      <script type="application/ld+json">{JSON.stringify(jsonLd)}</script>

      {/*
        Server-rendered, screen-reader-accessible description of the page. The
        interactive demo below is client-only (it needs WebAssembly), so this
        section is what gives crawlers and AI agents a real <h1> and a truthful
        summary of the page instead of an empty "loading" shell. It mirrors the
        copy shown in the app's pre-boot panels.
      */}
      <section className="sr-only" aria-label="About SimpleOS">
        <h1>
          SimpleOS: a Unix-like operating system that runs in your browser
        </h1>
        <p>
          SimpleOS is a from-scratch operating-system kernel written in C and
          x86 assembly. This page boots the real OS image inside a browser-based
          x86 emulator (v86) — no install and no virtual machine. It implements
          processes with round-robin preemptive scheduling, virtual memory with
          copy-on-write paging, 23 syscalls via <code>int 0x80</code>, pipes,
          signals, a filesystem, and an interactive shell, in roughly 8,000
          lines of code.
        </p>
        <h2>Why it exists</h2>
        <p>
          It is a learning project — built to understand how operating systems
          actually work by writing one from the ground up: boot flow, memory,
          interrupts, process behavior, and the low-level tradeoffs in between.
          It is a proof of concept, not a production OS.
        </p>
        <h2>How it works</h2>
        <p>
          A small custom kernel is assembled and packaged into a bootable ISO
          via GRUB, then run here inside a browser emulator. Boot the demo to
          get an interactive shell, keyboard capture, and live kernel telemetry.
        </p>
        <p>
          Source code is on <a href={siteConfig.repoUrl}>GitHub</a>. Built
          by <a href={siteConfig.author.url}>Zachary Roth</a>.
        </p>
      </section>

      <noscript>
        <div
          style={{
            position: "fixed",
            inset: 0,
            zIndex: 50,
            display: "flex",
            flexDirection: "column",
            alignItems: "center",
            justifyContent: "center",
            gap: "12px",
            background: "#0a0a12",
            color: "#eef6ff",
            padding: "24px",
            textAlign: "center",
            fontFamily: "monospace",
          }}
        >
          <strong>
            SimpleOS needs JavaScript to boot the in-browser emulator.
          </strong>
          <span style={{ maxWidth: "46ch", opacity: 0.7 }}>
            SimpleOS is a Unix-like OS built from scratch in C and x86 assembly.
            Enable JavaScript to run it here, or read the source on GitHub.
          </span>
          <a href={siteConfig.repoUrl} style={{ color: "#80e9ff" }}>
            View SimpleOS on GitHub
          </a>
        </div>
      </noscript>

      <EmulatorClient />
    </>
  );
}
