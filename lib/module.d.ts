export class audio
{
    unlink(): void;
    list(): string[];
    link(target: string, mode: "input" | "output"): boolean;
}
