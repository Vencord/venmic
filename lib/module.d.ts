export class audio
{
    unlink(): void;
    link(target: string): boolean;
    list(): { name: string; speaker: boolean }[];
}
