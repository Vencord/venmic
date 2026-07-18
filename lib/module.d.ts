type DefaultProps = 'node.name' | 'application.name';

type LiteralUnion<
	LiteralType,
	BaseType extends string,
> = LiteralType | (BaseType & Record<never, never>);

type Optional<
    Type, 
    Key extends keyof Type
> = Partial<Pick<Type, Key>> & Omit<Type, Key>;

export type Node<T extends string = never> = Record<LiteralUnion<T, string>, string>;

export interface LinkData
{
    include: Node[];
    exclude: Node[];

    only_speakers?: boolean;
    only_default_speakers?: boolean;

    ignore_devices?: boolean;
    
    workaround?: Node[];
    legacy_workaround?: boolean;
}

export class PatchBay
{
    unlink(): void;
    
    list<T extends string = DefaultProps>(props?: T[]): Node<T>[];
    link(data: Optional<LinkData, "exclude"> | Optional<LinkData, "include">): boolean;

    static hasPipeWire(): boolean;
}
