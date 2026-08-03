import { siteConfig } from "@/lib/site";
import type { MetadataRoute } from "next";

// Required for `output: "export"` — emit a static /sitemap.xml at build time.
export const dynamic = "force-static";

/**
 * Generates /sitemap.xml.
 *
 * The site is a single page, so one URL entry is correct. A sitemap *index*
 * (a sitemap of sitemaps) is only warranted past ~50,000 URLs and would be
 * wrong here. The Open Graph image is attached via the image extension so the
 * homepage's primary visual is discoverable even though it is generated.
 */
export default function sitemap(): MetadataRoute.Sitemap {
  return [
    {
      url: `${siteConfig.url}/`,
      lastModified: new Date(),
      changeFrequency: "monthly",
      priority: 1,
      images: [`${siteConfig.url}/og.png`],
    },
    {
      url: `${siteConfig.url}/learn`,
      lastModified: new Date(),
      changeFrequency: "monthly",
      priority: 0.8,
      images: [`${siteConfig.url}/og.png`],
    },
  ];
}
