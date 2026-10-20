import { chromium, Browser, Page } from "playwright";

export class WebAutomator {
    private browser: Browser | null = null;

    async initialize() {
        this.browser = await chromium.launch({
            headless: true,
            args: ["--disable-dev-shm-usage", "--no-sandbox"]
        });
    }

    async scanTarget(url: string) {
        if (!this.browser) throw new Error("Browser not initialized");
        const page = await this.browser.newPage();

        // Integrity evasion: mask playwright fingerprints
        await page.addInitScript(() => {
            Object.defineProperty(navigator, 'webdriver', { get: () => undefined });
        });

        await page.goto(url, { waitUntil: "networkidle" });

        const intel = await page.evaluate(() => {
            return {
                title: document.title,
                forms: Array.from(document.querySelectorAll("form")).map(f => ({
                    action: f.action,
                    method: f.method,
                    inputs: Array.from(f.querySelectorAll("input")).map(i => i.name)
                })),
                scripts: Array.from(document.querySelectorAll("script")).map(s => s.src).filter(src => src),
                cookies: document.cookie
            };
        });

        return intel;
    }

    async dispose() {
        if (this.browser) await this.browser.close();
    }
}
