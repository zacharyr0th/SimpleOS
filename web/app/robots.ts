import { siteConfig } from "@/lib/site";
import type { MetadataRoute } from "next";

// Required for `output: "export"` — emit a static /robots.txt at build time.
export const dynamic = "force-static";

/**
 * Generates /robots.txt (RFC 9309). Crawling is fully allowed — this is a
 * public, single-page site — and the sitemap + canonical host are advertised so
 * crawlers can discover the canonical URL set in one fetch.
 */
export default function robots(): MetadataRoute.Robots {
  return {
    rules: {
      userAgent: "*",
      allow: "/",
    },
    sitemap: `${siteConfig.url}/sitemap.xml`,
    host: siteConfig.url,
  };
}
