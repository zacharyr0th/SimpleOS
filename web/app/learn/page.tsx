import type { Metadata } from "next";
import Link from "next/link";
import { siteConfig } from "@/lib/site";

export const metadata: Metadata = {
  title: "Learn operating systems with SimpleOS",
  description:
    "A teaching guide for SimpleOS: the browser-runnable x86 kernel with processes, virtual memory, syscalls, pipes, signals, a filesystem, and a shell.",
  alternates: {
    canonical: "/learn",
  },
  openGraph: {
    type: "article",
    title: "Learn operating systems with SimpleOS",
    description:
      "Use SimpleOS as a compact OS-dev teaching project: boot flow, memory, syscalls, processes, filesystems, and shell behavior.",
    url: "/learn",
    siteName: siteConfig.name,
    images: ["/og.png"],
  },
};

const learningPath = [
  {
    title: "Boot flow",
    files: "src/arch/i386/boot.s, linker.ld, boot/grub/grub.cfg",
    note: "Trace how GRUB transfers control, the stack is prepared, and kernel_main starts.",
  },
  {
    title: "Memory",
    files: "src/mm/pmm.c, src/mm/vmm.c, src/mm/kmalloc.c",
    note: "Study physical allocation, paging, copy-on-write fork, and the kernel heap.",
  },
  {
    title: "Processes",
    files: "src/kernel/process.c, src/kernel/scheduler.c",
    note: "Follow PCB lifecycle, preemptive round-robin scheduling, context switching, and user processes.",
  },
  {
    title: "Syscalls",
    files: "src/kernel/syscall.c, userspace/ulib.h",
    note: "Map int 0x80 dispatch to user-space wrappers and shell-visible behavior.",
  },
  {
    title: "Shell and filesystem",
    files: "userspace/shell.c, src/fs/fs.c",
    note: "Inspect built-ins, pipes, redirection, job control, and the RAM filesystem.",
  },
];

const demoCommands = [
  "help",
  "ps",
  "ls",
  "ls /bin",
  "echo hello | wc",
  "cat /bin/hello",
];

const jsonLd = {
  "@context": "https://schema.org",
  "@type": "LearningResource",
  name: "Learn operating systems with SimpleOS",
  url: `${siteConfig.url}/learn`,
  description:
    "A teaching guide for SimpleOS: the browser-runnable x86 kernel with processes, virtual memory, syscalls, pipes, signals, a filesystem, and a shell.",
  learningResourceType: "Teaching guide",
  teaches: [
    "operating system boot flow",
    "x86 kernel development",
    "virtual memory",
    "syscalls",
    "process scheduling",
    "filesystem design",
  ],
  codeRepository: siteConfig.repoUrl,
  isAccessibleForFree: true,
};

export default function LearnPage() {
  return (
    <main className="min-h-screen bg-[#0a0a12] text-[#eef6ff]">
      <script type="application/ld+json">{JSON.stringify(jsonLd)}</script>
      <div className="mx-auto max-w-5xl px-6 py-10 sm:py-16">
        <Link
          href="/"
          className="inline-flex items-center gap-2 font-mono text-sm text-white/62 transition-colors hover:text-white"
        >
          Back to the emulator
        </Link>

        <section className="pt-12">
          <p className="font-mono text-xs uppercase tracking-[0.22em] text-cyan-200/70">
            OS-dev teaching guide
          </p>
          <h1 className="mt-4 max-w-4xl text-4xl font-semibold leading-tight tracking-tight sm:text-6xl">
            Learn operating-system internals by tracing a small working kernel.
          </h1>
          <p className="mt-6 max-w-3xl text-base leading-8 text-white/68 sm:text-lg">
            SimpleOS is intentionally small enough to read but complete enough
            to show the core ideas: boot, memory, processes, syscalls, pipes,
            signals, a filesystem, and a shell. The browser demo makes the
            first run zero-setup; the repository keeps the real ISO and QEMU
            path available for deeper work.
          </p>
        </section>

        <section className="mt-12 grid gap-4 sm:grid-cols-3" aria-label="Project summary">
          <div className="rounded-lg border border-white/10 bg-white/[0.04] p-5">
            <div className="font-mono text-xs uppercase tracking-[0.18em] text-cyan-200">
              CPU
            </div>
            <h2 className="mt-4 font-mono text-base font-semibold">Architecture</h2>
            <p className="mt-2 text-sm leading-6 text-white/62">
              C and x86 assembly, GRUB boot, ring 0 to ring 3, and 23 syscalls.
            </p>
          </div>
          <div className="rounded-lg border border-white/10 bg-white/[0.04] p-5">
            <div className="font-mono text-xs uppercase tracking-[0.18em] text-cyan-200">
              RUN
            </div>
            <h2 className="mt-4 font-mono text-base font-semibold">Try it</h2>
            <p className="mt-2 text-sm leading-6 text-white/62">
              Boot the live ISO in the browser and run shell commands without a toolchain.
            </p>
          </div>
          <div className="rounded-lg border border-white/10 bg-white/[0.04] p-5">
            <div className="font-mono text-xs uppercase tracking-[0.18em] text-cyan-200">
              SRC
            </div>
            <h2 className="mt-4 font-mono text-base font-semibold">Read it</h2>
            <p className="mt-2 text-sm leading-6 text-white/62">
              Follow each subsystem from the README into the exact source files.
            </p>
          </div>
        </section>

        <section className="mt-14" aria-labelledby="path-heading">
          <h2 id="path-heading" className="text-2xl font-semibold tracking-tight">
            Suggested reading path
          </h2>
          <div className="mt-5 divide-y divide-white/10 rounded-lg border border-white/10">
            {learningPath.map((item, index) => (
              <article key={item.title} className="grid gap-3 p-5 sm:grid-cols-[3rem_1fr]">
                <div className="font-mono text-sm text-cyan-200/80">
                  {String(index + 1).padStart(2, "0")}
                </div>
                <div>
                  <h3 className="font-mono text-base font-semibold">{item.title}</h3>
                  <p className="mt-1 font-mono text-xs text-white/48">{item.files}</p>
                  <p className="mt-3 max-w-3xl text-sm leading-6 text-white/64">
                    {item.note}
                  </p>
                </div>
              </article>
            ))}
          </div>
        </section>

        <section className="mt-14" aria-labelledby="commands-heading">
          <h2 id="commands-heading" className="text-2xl font-semibold tracking-tight">
            Browser demo commands
          </h2>
          <div className="mt-5 flex flex-wrap gap-2">
            {demoCommands.map((command) => (
              <code
                key={command}
                className="rounded-md border border-white/10 bg-black/40 px-3 py-2 font-mono text-sm text-white/78"
              >
                {command}
              </code>
            ))}
          </div>
        </section>

        <div className="mt-14 flex flex-wrap gap-4">
          <Link
            href="/"
            className="inline-flex items-center gap-2 rounded-md bg-cyan-200 px-4 py-2 text-sm font-medium text-black"
          >
            Boot the browser demo
          </Link>
          <a
            href={siteConfig.repoUrl}
            target="_blank"
            rel="noopener noreferrer"
            className="inline-flex items-center gap-2 rounded-md border border-white/14 px-4 py-2 text-sm font-medium text-white/82 transition-colors hover:bg-white/[0.06] hover:text-white"
          >
            View source
          </a>
        </div>
      </div>
    </main>
  );
}
