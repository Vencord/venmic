type DefaultProps = 'node.name' | 'application.name';

type LiteralUnion<
	LiteralType,
	BaseType extends string,
> = LiteralType | (BaseType & Record<never, never>);

export class PatchBay
{
    unlink(): void;
    
    list<T extends string = DefaultProps>(props?: T[]): Record<LiteralUnion<T, string>, string>[];
    link(data: {key: string, value: string, mode: "include" | "exclude"}): boolean;

    static hasPipeWire(): boolean;
}
