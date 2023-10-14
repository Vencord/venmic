export interface Props
{
    "application.process.binary": string;
    "application.process.id": string;
    "node.name": string;
}

export class PatchBay
{
    list(): Props[];
    
    unlink(): void;
    link(key: keyof Props, value: string, mode: "include" | "exclude"): boolean;

    static hasPipeWire(): boolean;
}
