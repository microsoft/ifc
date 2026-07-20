# Copyright Microsoft Corporation.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('quote-header-unit', 'angle-header-unit', 'tamper-content', 'tamper-hash', 'unsorted-toc',
                 'bad-path-offset', 'overlap-extents', 'junction')]
    [string]$Mode,

    [Parameter(Mandatory = $true)]
    [string]$InputFile,

    [Parameter(Mandatory = $true)]
    [string]$OutputFile
)

$ErrorActionPreference = 'Stop'

# IFC header and ArchiveMember offsets used by deliberate fixture mutation.
$contentHashOffset = 4
$hashedContentsOffset = 36
$stringTableFieldOffset = 44
$stringTableSizeFieldOffset = 48
$unitIndexOffset = 52
$tocFieldOffset = 64
$memberCountFieldOffset = 68
$archiveMemberSize = 16
$memberPathFieldOffset = 4
$memberPayloadFieldOffset = 8

if ($Mode -eq 'junction')
{
    New-Item -ItemType Junction -Path $OutputFile -Target $InputFile | Out-Null
    exit 0
}

$bytes = [IO.File]::ReadAllBytes($InputFile)

# Recompute the outer IFC digest after deliberately changing authenticated metadata.
function Set-ContentHash
{
    param([byte[]]$Image)

    $sha = [Security.Cryptography.SHA256]::Create()
    try
    {
        $digest = $sha.ComputeHash($Image, $hashedContentsOffset, $Image.Length - $hashedContentsOffset)
        [Array]::Copy($digest, 0, $Image, $contentHashOffset, $digest.Length)
    }
    finally
    {
        $sha.Dispose()
    }
}

# Give the fixture a realistic delimited header-unit canonical name while preserving its layout.
function Set-HeaderUnitName
{
    param(
        [byte[]]$Image,
        [ValidateSet('quote', 'angle')]
        [string]$Form
    )

    $unit = [BitConverter]::ToUInt32($Image, $unitIndexOffset)
    $nameOffset = $unit -shr 3
    $tableOffset = [BitConverter]::ToUInt32($Image, $stringTableFieldOffset)
    $tableSize = [BitConverter]::ToUInt32($Image, $stringTableSizeFieldOffset)
    $start = $tableOffset + $nameOffset
    $end = $start
    while ($end -lt $tableOffset + $tableSize -and $Image[$end] -ne 0)
    {
        ++$end
    }
    if ($end -eq $tableOffset + $tableSize)
    {
        throw 'member canonical name is not terminated'
    }
    $name = [Text.Encoding]::UTF8.GetString($Image, $start, $end - $start)
    $canonical = if ($Form -eq 'quote') { '"' + $name + '"' } else { '<' + $name + '>' }
    $encoded = [Text.Encoding]::UTF8.GetBytes($canonical)
    if ($start + $encoded.Length -ge $tableOffset + $tableSize)
    {
        throw 'string table has no room for a delimited fixture name'
    }
    [Array]::Copy($encoded, 0, $Image, $start, $encoded.Length)
    $Image[$start + $encoded.Length] = 0

    $unit = ($unit -band 0xfffffff8) -bor 3
    [BitConverter]::GetBytes([uint32]$unit).CopyTo($Image, $unitIndexOffset)
    Set-ContentHash $Image
}

switch ($Mode)
{
    'quote-header-unit'
    {
        Set-HeaderUnitName $bytes quote
    }
    'angle-header-unit'
    {
        Set-HeaderUnitName $bytes angle
    }
    'tamper-content'
    {
        $bytes[$bytes.Length - 1] = $bytes[$bytes.Length - 1] -bxor 1
    }
    'tamper-hash'
    {
        $bytes[$contentHashOffset] = $bytes[$contentHashOffset] -bxor 1
    }
    'unsorted-toc'
    {
        $toc = [BitConverter]::ToUInt32($bytes, $tocFieldOffset)
        $count = [BitConverter]::ToUInt32($bytes, $memberCountFieldOffset)
        if ($count -lt 2)
        {
            throw 'unsorted-toc requires at least two archive members'
        }
        $first = [byte[]]::new($archiveMemberSize)
        [Array]::Copy($bytes, $toc, $first, 0, $archiveMemberSize)
        [Array]::Copy($bytes, $toc + $archiveMemberSize, $bytes, $toc, $archiveMemberSize)
        [Array]::Copy($first, 0, $bytes, $toc + $archiveMemberSize, $archiveMemberSize)
        Set-ContentHash $bytes
    }
    'bad-path-offset'
    {
        $toc = [BitConverter]::ToUInt32($bytes, $tocFieldOffset)
        [BitConverter]::GetBytes([uint32]::MaxValue).CopyTo($bytes, $toc + $memberPathFieldOffset)
        Set-ContentHash $bytes
    }
    'overlap-extents'
    {
        $toc = [BitConverter]::ToUInt32($bytes, $tocFieldOffset)
        $count = [BitConverter]::ToUInt32($bytes, $memberCountFieldOffset)
        if ($count -lt 2)
        {
            throw 'overlap-extents requires at least two archive members'
        }
        # ArchiveMember::offset is its third 32-bit field; make member 2 start at member 1.
        [Array]::Copy($bytes, $toc + $memberPayloadFieldOffset, $bytes,
                  $toc + $archiveMemberSize + $memberPayloadFieldOffset, 4)
        Set-ContentHash $bytes
    }
}

[IO.File]::WriteAllBytes($OutputFile, $bytes)
