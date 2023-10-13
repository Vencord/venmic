export class PatchBay
{
    unlink(): void;
    list(): string[];
    link(target: string, mode: "include" | "exclude"): boolean;
}
