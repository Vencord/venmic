export interface Props
{
    "application.process.binary": string;
    "application.process.id": string;
    "node.name": string;
}

export class PatchBay
{
    unlink(): void;
    list(): Props[];
    
    link(key: string, value: string, mode: "include" | "exclude"): boolean;
}
